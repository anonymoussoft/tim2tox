// IrcClientManager.cpp
#include "IrcClientManager.h"
#include "V2TIMLog.h"

#ifdef _WIN32
  #include <winsock2.h>
  #include <ws2tcpip.h>
  #pragma comment(lib, "ws2_32.lib")
  typedef int socklen_t;
  typedef long long ssize_t;
  #define IRC_CLOSE_SOCKET(s) closesocket(s)
  #define IRC_ERRNO WSAGetLastError()
  #define IRC_EINPROGRESS WSAEWOULDBLOCK
  static void irc_set_nonblocking(int sock, bool nonblocking) {
      u_long mode = nonblocking ? 1 : 0;
      ioctlsocket(sock, FIONBIO, &mode);
  }
#else
  #include <sys/socket.h>
  #include <netinet/in.h>
  #include <arpa/inet.h>
  #include <netdb.h>
  #include <unistd.h>
  #include <fcntl.h>
  #include <sys/ioctl.h>
  #define IRC_CLOSE_SOCKET(s) close(s)
  #define IRC_ERRNO errno
  #define IRC_EINPROGRESS EINPROGRESS
  static void irc_set_nonblocking(int sock, bool nonblocking) {
      int flags = fcntl(sock, F_GETFL, 0);
      if (nonblocking) {
          fcntl(sock, F_SETFL, flags | O_NONBLOCK);
      } else {
          fcntl(sock, F_SETFL, flags & ~O_NONBLOCK);
      }
  }
#endif

#include <cstring>
#include <cstdio>
#include <sstream>
#include <algorithm>
#include <vector>
#include <chrono>
#include <thread>
#include <mutex>
#include <algorithm>

// OpenSSL includes
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/bio.h>

#if defined(_WIN32) || defined(__WIN32__) || defined(WIN32)
#define MSG_NOSIGNAL 0
#endif

#if defined(__FreeBSD__) && !defined(MSG_NOSIGNAL)
#define MSG_NOSIGNAL 0x20000
#endif

// macOS already defines MSG_NOSIGNAL, so don't redefine it
#if defined(__MACH__) && !defined(MSG_NOSIGNAL)
#define MSG_NOSIGNAL 0
#endif

IrcClientManager& IrcClientManager::getInstance() {
    static IrcClientManager instance;
    return instance;
}

IrcClientManager::IrcClientManager() {
}

IrcClientManager::~IrcClientManager() {
    shutdown();
}

int IrcClientManager::connectToServer(const std::string& server, int port, bool use_ssl) {
    struct addrinfo hints = {};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    struct addrinfo* result = nullptr;
    std::string port_str = std::to_string(port);
    
    int ret = getaddrinfo(server.c_str(), port_str.c_str(), &hints, &result);
    if (ret != 0) {
        V2TIM_LOG(kError, "[IRC] getaddrinfo failed: {}", gai_strerror(ret));
        return -1;
    }

    int sock = -1;
    for (struct addrinfo* rp = result; rp != nullptr; rp = rp->ai_next) {
        sock = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (sock < 0) {
            continue;
        }

        // Set non-blocking
        irc_set_nonblocking(sock, true);

        if (connect(sock, rp->ai_addr, rp->ai_addrlen) == 0) {
            break;
        }

        // Check if connection is in progress
        if (IRC_ERRNO == IRC_EINPROGRESS) {
            fd_set write_fds;
            FD_ZERO(&write_fds);
            FD_SET(sock, &write_fds);
            struct timeval timeout;
            timeout.tv_sec = 5;
            timeout.tv_usec = 0;
            
            if (select(sock + 1, nullptr, &write_fds, nullptr, &timeout) > 0) {
                int error = 0;
                socklen_t len = sizeof(error);
                if (getsockopt(sock, SOL_SOCKET, SO_ERROR, (char*)&error, &len) == 0 && error == 0) {
                    break;
                }
            }
        }

        IRC_CLOSE_SOCKET(sock);
        sock = -1;
    }

    freeaddrinfo(result);

    if (sock < 0) {
        V2TIM_LOG(kError, "[IRC] Failed to connect to {}:{}", server, port);
        return -1;
    }

    // Set back to blocking
    irc_set_nonblocking(sock, false);

    V2TIM_LOG(kInfo, "[IRC] Connected to {}:{}{}", server, port, use_ssl ? " (SSL)" : "");
    return sock;
}

bool IrcClientManager::sendIrcCommand(int sock, const std::string& command) {
    if (sock < 0) {
        return false;
    }
    
    std::string cmd = command;
    if (cmd.back() != '\n') {
        cmd += "\n";
    }
    
    ssize_t sent = send(sock, cmd.c_str(), cmd.length(), MSG_NOSIGNAL);
    if (sent < 0) {
        V2TIM_LOG(kError, "[IRC] Failed to send command: {}", cmd);
        return false;
    }

    V2TIM_LOG(kInfo, "[IRC] Sent: {}", cmd);
    return true;
}

// Send IRC command (with SSL support)
bool IrcClientManager::sendIrcCommand(IrcChannel* channel, const std::string& command) {
    if (channel->sock_fd < 0) {
        return false;
    }
    
    std::string cmd = command;
    if (cmd.back() != '\n') {
        cmd += "\n";
    }
    
    ssize_t sent;
    if (channel->use_ssl && channel->ssl) {
        sent = sslSend((SSL*)channel->ssl, cmd.c_str(), cmd.length());
    } else {
        sent = send(channel->sock_fd, cmd.c_str(), cmd.length(), MSG_NOSIGNAL);
    }
    
    if (sent < 0) {
        V2TIM_LOG(kError, "[IRC] Failed to send command: {}", cmd);
        return false;
    }

    V2TIM_LOG(kInfo, "[IRC] Sent: {}", cmd);
    return true;
}

void IrcClientManager::handlePing(IrcChannel* channel, const std::string& line) {
    // Respond to PING with PONG
    if (line.length() > 4 && line.substr(0, 4) == "PING") {
        std::string pong = "PONG" + line.substr(4);
        sendIrcCommand(channel, pong);
    }
}

void IrcClientManager::handlePrivmsg(IrcChannel* channel, const std::string& line) {
    // Parse PRIVMSG: :nick!user@host PRIVMSG #channel :message
    size_t colon1 = line.find(':');
    if (colon1 == std::string::npos) return;
    
    size_t space1 = line.find(' ', colon1 + 1);
    if (space1 == std::string::npos) return;
    
    size_t exclamation = line.find('!', colon1 + 1);
    std::string sender_nick;
    if (exclamation != std::string::npos && exclamation < space1) {
        sender_nick = line.substr(colon1 + 1, exclamation - colon1 - 1);
    } else {
        sender_nick = line.substr(colon1 + 1, space1 - colon1 - 1);
    }
    
    // Find PRIVMSG
    size_t privmsg_pos = line.find("PRIVMSG", space1);
    if (privmsg_pos == std::string::npos) return;
    
    // Find channel name
    size_t channel_start = line.find(' ', privmsg_pos + 7) + 1;
    if (channel_start == std::string::npos || channel_start >= line.length()) return;
    
    size_t colon2 = line.find(':', channel_start);
    if (colon2 == std::string::npos) return;
    
    std::string msg_channel = line.substr(channel_start, colon2 - channel_start);
    // Remove leading/trailing spaces
    msg_channel.erase(0, msg_channel.find_first_not_of(" \t"));
    msg_channel.erase(msg_channel.find_last_not_of(" \t") + 1);
    
    // Check if this message is for our channel
    if (msg_channel != channel->channel) {
        return;
    }
    
    // Extract message
    std::string message = line.substr(colon2 + 1);
    // Remove trailing \r\n
    while (!message.empty() && (message.back() == '\n' || message.back() == '\r')) {
        message.pop_back();
    }
    
    // Call callback to send to Tox
    if (tox_message_callback_) {
        tox_message_callback_(channel->group_id, sender_nick, message);
    }
    
    V2TIM_LOG(kInfo, "[IRC] PRIVMSG from {} in {}: {}", sender_nick, channel->channel, message);
}

void IrcClientManager::processIrcMessage(IrcChannel* channel, const std::string& line) {
    if (line.empty()) return;
    
    // Handle PING
    if (line.length() >= 4 && line.substr(0, 4) == "PING") {
        handlePing(channel, line);
        return;
    }
    
    // Handle PRIVMSG
    if (line.find("PRIVMSG") != std::string::npos) {
        handlePrivmsg(channel, line);
        return;
    }
    
    // Handle JOIN confirmation
    if (line.find("JOIN") != std::string::npos) {
        // Check if it's our own JOIN or another user's JOIN
        if (line.find(channel->channel) != std::string::npos) {
            // Check if it's our own JOIN (no : prefix before JOIN)
            size_t join_pos = line.find("JOIN");
            if (join_pos > 0 && line[join_pos - 1] == ' ') {
                // This is our own JOIN
                V2TIM_LOG(kInfo, "[IRC] Successfully joined {}", channel->channel);
                channel->connected = true;
                updateConnectionStatus(channel, ConnectionStatus::Connected, "Connected to channel");
                // Request user list
                sendIrcCommand(channel, "NAMES " + channel->channel);
            } else {
                // Another user joined
                handleJoinPart(channel, line, true);
            }
        }
        return;
    }
    
    // Handle PART
    if (line.find("PART") != std::string::npos && line.find(channel->channel) != std::string::npos) {
        handleJoinPart(channel, line, false);
        return;
    }
    
    // Handle NAMES response (353 RPL_NAMREPLY)
    if (line.length() >= 3 && line.substr(0, 3) == "353") {
        handleNamesResponse(channel, line);
        return;
    }
    
    // Handle end of NAMES list (366 RPL_ENDOFNAMES)
    if (line.length() >= 3 && line.substr(0, 3) == "366") {
        // NAMES list complete
        return;
    }
    
    // Handle CAP responses (for SASL)
    if (line.find("CAP") != std::string::npos) {
        if (line.find("ACK :sasl") != std::string::npos || line.find("ACK sasl") != std::string::npos) {
            // Server acknowledged SASL capability
            V2TIM_LOG(kInfo, "[IRC] SASL capability acknowledged, starting authentication");
            sendIrcCommand(channel, "AUTHENTICATE PLAIN");
        } else if (line.find("NAK") != std::string::npos) {
            V2TIM_LOG(kError, "[IRC] SASL capability not available");
        } else if (line.find("LS") != std::string::npos && line.find("sasl") != std::string::npos) {
            // Server lists capabilities, request SASL
            sendIrcCommand(channel, "CAP REQ :sasl");
        }
        return;
    }
    
    // Handle AUTHENTICATE responses
    if (line.find("AUTHENTICATE") != std::string::npos) {
        handleSaslAuth(channel, line);
        return;
    }
    
    // Handle numeric responses (001 = welcome, etc.)
    if (line.length() >= 3 && line[0] >= '0' && line[0] <= '9') {
        std::string code = line.substr(0, 3);
        if (code == "001" || code == "002" || code == "003") {
            // Server welcome, but only join channel if SASL is done (or not using SASL)
            if (!channel->use_sasl || channel->sasl_authenticated) {
                std::string join_cmd = "JOIN " + channel->channel;
                if (!channel->password.empty()) {
                    join_cmd += " " + channel->password;
                }
                sendIrcCommand(channel, join_cmd);
            }
        } else if (code == "900" || code == "903") {
            // SASL authentication successful
            V2TIM_LOG(kInfo, "[IRC] SASL authentication successful");
            channel->sasl_authenticated = true;
            // End CAP negotiation
            sendIrcCommand(channel, "CAP END");
            // Now join channel
            std::string join_cmd = "JOIN " + channel->channel;
            if (!channel->password.empty()) {
                join_cmd += " " + channel->password;
            }
            sendIrcCommand(channel->sock_fd, join_cmd);
        } else if (code == "904" || code == "905") {
            // SASL authentication failed
            V2TIM_LOG(kError, "[IRC] SASL authentication failed: {}", line);
            updateConnectionStatus(channel, ConnectionStatus::Error, "SASL authentication failed");
            channel->should_stop = true;
        } else if (code == "433") {
            // ERR_NICKNAMEINUSE - Nickname already in use
            handleNickConflict(channel);
        } else if (code == "001" || code == "002" || code == "003") {
            // Server welcome messages
            updateConnectionStatus(channel, ConnectionStatus::Connected, "Connected to server");
        } else if (code == "004" || code == "005") {
            // Server capabilities/info
            // Continue
        } else if (code == "471" || code == "473" || code == "474" || code == "475") {
            // Channel errors
            std::string error_msg = "Channel error: " + line;
            updateConnectionStatus(channel, ConnectionStatus::Error, error_msg);
            V2TIM_LOG(kError, "[IRC] {}", error_msg);
        }
        return;
    }
    
    V2TIM_LOG(kInfo, "[IRC] Unhandled message: {}", line);
}

void IrcClientManager::channelThread(IrcChannel* channel) {
    V2TIM_LOG(kInfo, "[IRC] Thread started for channel {}", channel->channel);
    
    updateConnectionStatus(channel, ConnectionStatus::Connecting, "Connecting to server");
    
    // Connect to server (with retry logic)
    while (!channel->should_stop) {
        channel->sock_fd = connectToServer(channel->server, channel->port, channel->use_ssl);
        if (channel->sock_fd >= 0) {
            // Perform SSL handshake if needed
            if (channel->use_ssl) {
                if (!performSslHandshake(channel)) {
                    V2TIM_LOG(kError, "[IRC] SSL handshake failed");
                    IRC_CLOSE_SOCKET(channel->sock_fd);
                    channel->sock_fd = -1;
                    // Try to reconnect
                    attemptReconnect(channel);
                    if (channel->should_stop) {
                        return;
                    }
                    std::this_thread::sleep_for(std::chrono::seconds(IrcChannel::reconnect_delay_seconds));
                    continue;
                }
            }
            
            channel->reconnect_attempts = 0; // Reset on successful connection
            break;
        }
        
        // Connection failed, try to reconnect
        attemptReconnect(channel);
        if (channel->should_stop) {
            return;
        }
        
        // Wait before retry
        std::this_thread::sleep_for(std::chrono::seconds(IrcChannel::reconnect_delay_seconds));
    }
    
    if (channel->sock_fd < 0) {
        updateConnectionStatus(channel, ConnectionStatus::Error, "Failed to connect to server");
        return;
    }
    
    // Generate nickname
    std::string nick = generateNickname(channel);
    
    // If using SASL, request CAP first
    if (channel->use_sasl) {
        sendIrcCommand(channel, "CAP REQ :sasl");
        updateConnectionStatus(channel, ConnectionStatus::Authenticating, "Starting SASL authentication");
    }
    
    // Send NICK and USER commands
    sendIrcCommand(channel, "NICK " + nick);
    sendIrcCommand(channel, "USER " + nick + " 8 * :" + nick);
    
    // Main loop
    char buffer[4096];
    std::string line_buffer;
    
    while (!channel->should_stop) {
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(channel->sock_fd, &read_fds);
        
        struct timeval timeout;
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;
        
        int ret = select(channel->sock_fd + 1, &read_fds, nullptr, nullptr, &timeout);
        if (ret < 0) {
#ifdef _WIN32
            break;
#else
            if (errno == EINTR) continue;
            break;
#endif
        }
        
        if (ret == 0) {
            // Timeout, check if still connected
            continue;
        }
        
        if (FD_ISSET(channel->sock_fd, &read_fds)) {
            ssize_t n;
            if (channel->use_ssl && channel->ssl) {
                n = sslRecv((SSL*)channel->ssl, buffer, sizeof(buffer) - 1);
            } else {
                n = recv(channel->sock_fd, buffer, sizeof(buffer) - 1, 0);
            }
            if (n <= 0) {
                if (n == 0) {
                    V2TIM_LOG(kInfo, "[IRC] Connection closed for {}", channel->channel);
                    updateConnectionStatus(channel, ConnectionStatus::Disconnected, "Connection closed");
                } else {
                    int sock_err = IRC_ERRNO;
                    V2TIM_LOG(kError, "[IRC] recv error for {}: error code {}", channel->channel, sock_err);
                    updateConnectionStatus(channel, ConnectionStatus::Error, std::string("Recv error: ") + std::to_string(sock_err));
                }
                
                // Try to reconnect if not explicitly stopped
                if (!channel->should_stop) {
                    attemptReconnect(channel);
                    if (channel->sock_fd < 0) {
                        // Reconnect failed, wait and retry in next iteration
                        std::this_thread::sleep_for(std::chrono::seconds(IrcChannel::reconnect_delay_seconds));
                        continue;
                    }
                    // Reconnected, continue processing
                    continue;
                } else {
                    break;
                }
            }
            
            buffer[n] = '\0';
            line_buffer += buffer;
            
            // Process complete lines
            size_t pos;
            while ((pos = line_buffer.find('\n')) != std::string::npos) {
                std::string line = line_buffer.substr(0, pos);
                // Remove \r if present
                if (!line.empty() && line.back() == '\r') {
                    line.pop_back();
                }
                processIrcMessage(channel, line);
                line_buffer.erase(0, pos + 1);
            }
        }
    }
    
    // Cleanup
    if (channel->sock_fd >= 0) {
        IRC_CLOSE_SOCKET(channel->sock_fd);
        channel->sock_fd = -1;
    }
    channel->connected = false;
    updateConnectionStatus(channel, ConnectionStatus::Disconnected, "Thread ended");
    V2TIM_LOG(kInfo, "[IRC] Thread ended for channel {}", channel->channel);
}

bool IrcClientManager::connectChannel(const std::string& server, int port,
                                      const std::string& channel,
                                      const std::string& password,
                                      const std::string& group_id,
                                      const std::string& sasl_username,
                                      const std::string& sasl_password,
                                      bool use_ssl,
                                      const std::string& custom_nickname) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Check if channel already exists
    if (channels_.find(channel) != channels_.end()) {
        V2TIM_LOG(kError, "[IRC] Channel {} already connected", channel);
        return false;
    }
    
    auto irc_channel = std::make_unique<IrcChannel>();
    irc_channel->server = server;
    irc_channel->port = port;
    irc_channel->channel = channel;
    irc_channel->password = password;
    irc_channel->group_id = group_id;
    irc_channel->sasl_username = sasl_username;
    irc_channel->sasl_password = sasl_password;
    irc_channel->custom_nickname = custom_nickname;
    irc_channel->use_ssl = use_ssl;
    // Enable SASL if username is provided (password can be empty for some servers)
    irc_channel->use_sasl = !sasl_username.empty();
    irc_channel->sasl_authenticated = false;
    irc_channel->status = ConnectionStatus::Connecting;
    irc_channel->reconnect_attempts = 0;
    
    // Generate base nickname
    if (custom_nickname.empty()) {
        irc_channel->base_nickname = "ToxUser_" + channel.substr(1, 5);
        std::replace(irc_channel->base_nickname.begin(), irc_channel->base_nickname.end(), '#', 'x');
    } else {
        irc_channel->base_nickname = custom_nickname;
    }
    irc_channel->nickname_suffix = 0;
    
    bool use_sasl = !sasl_username.empty();
    
    // Start thread
    irc_channel->thread = std::make_unique<std::thread>(&IrcClientManager::channelThread, this, irc_channel.get());
    
    channels_[channel] = std::move(irc_channel);
    
    V2TIM_LOG(kInfo, "[IRC] Connecting to {}:{}, channel {}{}{}", server, port, channel,
              use_sasl ? " (with SASL)" : "",
              use_ssl ? " (with SSL)" : "");
    return true;
}

bool IrcClientManager::disconnectChannel(const std::string& channel) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = channels_.find(channel);
    if (it == channels_.end()) {
        return false;
    }
    
    IrcChannel* irc_channel = it->second.get();
    irc_channel->should_stop = true;
    
    // Cleanup SSL
    if (irc_channel->ssl) {
        SSL_shutdown((SSL*)irc_channel->ssl);
        SSL_free((SSL*)irc_channel->ssl);
        irc_channel->ssl = nullptr;
    }
    if (irc_channel->ssl_ctx) {
        SSL_CTX_free((SSL_CTX*)irc_channel->ssl_ctx);
        irc_channel->ssl_ctx = nullptr;
    }
    
    // Close socket to wake up thread
    if (irc_channel->sock_fd >= 0) {
        IRC_CLOSE_SOCKET(irc_channel->sock_fd);
        irc_channel->sock_fd = -1;
    }
    
    // Wait for thread
    if (irc_channel->thread && irc_channel->thread->joinable()) {
        irc_channel->thread->join();
    }
    
    channels_.erase(it);
    
    V2TIM_LOG(kInfo, "[IRC] Disconnected channel {}", channel);
    return true;
}

bool IrcClientManager::sendMessage(const std::string& channel, const std::string& message) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = channels_.find(channel);
    if (it == channels_.end() || !it->second->connected || it->second->sock_fd < 0) {
        return false;
    }
    
    std::string privmsg = "PRIVMSG " + channel + " :" + message;
    return sendIrcCommand(it->second.get(), privmsg);
}

// Internal helper to send message without locking (caller must hold mutex)
bool IrcClientManager::sendMessageUnlocked(const std::string& channel, const std::string& message) {
    auto it = channels_.find(channel);
    if (it == channels_.end() || !it->second->connected || it->second->sock_fd < 0) {
        return false;
    }
    
    std::string privmsg = "PRIVMSG " + channel + " :" + message;
    return sendIrcCommand(it->second.get(), privmsg);
}

bool IrcClientManager::isChannelConnected(const std::string& channel) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = channels_.find(channel);
    return it != channels_.end() && it->second->connected;
}

void IrcClientManager::setToxMessageCallback(std::function<void(const std::string&, const std::string&, const std::string&)> callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    tox_message_callback_ = callback;
}

void IrcClientManager::setToxGroupMessageCallback(std::function<void(const std::string&, const std::string&, const std::string&)> callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    tox_group_message_callback_ = callback;
}

void IrcClientManager::onToxGroupMessage(const std::string& group_id, const std::string& sender, const std::string& message) {
    // Call external callback if set (for dynamic library)
    if (tox_group_message_callback_) {
        tox_group_message_callback_(group_id, sender, message);
        return;
    }
    
    // Otherwise, forward directly to IRC
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Find channel by group_id
    for (const auto& pair : channels_) {
        if (pair.second->group_id == group_id) {
            // Send to IRC (use unlocked version since we already hold the lock)
            // Note: IRC server will automatically prepend sender info, so we just send the message
            sendMessageUnlocked(pair.first, message);
            return;
        }
    }
}

void IrcClientManager::shutdown() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    for (auto& pair : channels_) {
        pair.second->should_stop = true;
        if (pair.second->sock_fd >= 0) {
            IRC_CLOSE_SOCKET(pair.second->sock_fd);
            pair.second->sock_fd = -1;
        }
    }
    
    for (auto& pair : channels_) {
        if (pair.second->thread && pair.second->thread->joinable()) {
            pair.second->thread->join();
        }
    }
    
    channels_.clear();
}

void IrcClientManager::handleSaslAuth(IrcChannel* channel, const std::string& line) {
    if (line.find("AUTHENTICATE +") != std::string::npos) {
        // Server is ready for authentication data
        // Format: "\0username\0password" base64 encoded
        std::string auth_data = "\0" + channel->sasl_username + "\0" + channel->sasl_password;
        std::string encoded = base64Encode(auth_data);
        sendIrcCommand(channel, "AUTHENTICATE " + encoded);
        V2TIM_LOG(kInfo, "[IRC] Sent SASL authentication data");
    }
}

std::string IrcClientManager::base64Encode(const std::string& data) {
    static const char base64_chars[] = 
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    
    std::string encoded;
    int val = 0, valb = -6;
    
    for (unsigned char c : data) {
        val = (val << 8) + c;
        valb += 8;
        while (valb >= 0) {
            encoded.push_back(base64_chars[(val >> valb) & 0x3F]);
            valb -= 6;
        }
    }
    
    if (valb > -6) {
        encoded.push_back(base64_chars[((val << 8) >> (valb + 8)) & 0x3F]);
    }
    
    while (encoded.size() % 4) {
        encoded.push_back('=');
    }
    
    return encoded;
}

// 更新连接状态
void IrcClientManager::updateConnectionStatus(IrcChannel* channel, ConnectionStatus status, const std::string& message) {
    channel->status = status;
    if (connection_status_callback_) {
        connection_status_callback_(channel->channel, status, message);
    }
}

// 生成昵称
std::string IrcClientManager::generateNickname(IrcChannel* channel) {
    if (!channel->custom_nickname.empty() && channel->nickname_suffix == 0) {
        return channel->custom_nickname;
    }
    
    if (channel->nickname_suffix == 0) {
        return channel->base_nickname;
    }
    
    return channel->base_nickname + "_" + std::to_string(channel->nickname_suffix);
}

// 处理昵称冲突
void IrcClientManager::handleNickConflict(IrcChannel* channel) {
    channel->nickname_suffix++;
    std::string new_nick = generateNickname(channel);
    sendIrcCommand(channel, "NICK " + new_nick);
    V2TIM_LOG(kInfo, "[IRC] Nickname conflict, trying: {}", new_nick);
}

// 自动重连
void IrcClientManager::attemptReconnect(IrcChannel* channel) {
    if (channel->reconnect_attempts >= IrcChannel::max_reconnect_attempts) {
        updateConnectionStatus(channel, ConnectionStatus::Error, "Max reconnect attempts reached");
        channel->should_stop = true;
        return;
    }
    
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - channel->last_reconnect_time).count();
    
    if (elapsed < IrcChannel::reconnect_delay_seconds) {
        return; // Wait more
    }
    
    channel->reconnect_attempts++;
    channel->last_reconnect_time = now;
    
    updateConnectionStatus(channel, ConnectionStatus::Reconnecting, 
                          "Reconnecting (attempt " + std::to_string(channel->reconnect_attempts) + ")");
    
    // Close old socket
    if (channel->sock_fd >= 0) {
        IRC_CLOSE_SOCKET(channel->sock_fd);
        channel->sock_fd = -1;
    }
    
    // Try to reconnect
    channel->sock_fd = connectToServer(channel->server, channel->port, channel->use_ssl);
    if (channel->sock_fd < 0) {
        V2TIM_LOG(kError, "[IRC] Reconnect attempt {} failed for {}",
                  channel->reconnect_attempts, channel->channel);
        return;
    }
    
    // Perform SSL handshake if needed
    if (channel->use_ssl) {
        if (!performSslHandshake(channel)) {
            V2TIM_LOG(kError, "[IRC] SSL handshake failed on reconnect");
            IRC_CLOSE_SOCKET(channel->sock_fd);
            channel->sock_fd = -1;
            return;
        }
    }
    
    // Reset reconnect attempts on successful connection
    channel->reconnect_attempts = 0;
    channel->sasl_authenticated = false;
    
    // Re-send NICK and USER
    std::string nick = generateNickname(channel);
    if (channel->use_sasl) {
        sendIrcCommand(channel, "CAP REQ :sasl");
    }
    sendIrcCommand(channel, "NICK " + nick);
    sendIrcCommand(channel, "USER " + nick + " 8 * :" + nick);
    
    updateConnectionStatus(channel, ConnectionStatus::Connecting, "Reconnected, authenticating");
}

// 处理NAMES响应
void IrcClientManager::handleNamesResponse(IrcChannel* channel, const std::string& line) {
    // Format: :server 353 nickname = #channel :nick1 nick2 nick3
    // Or: :server 353 nickname @ #channel :@op1 +voice1 nick2
    size_t colon_pos = line.find_last_of(':');
    if (colon_pos == std::string::npos) return;
    
    std::string names_str = line.substr(colon_pos + 1);
    std::lock_guard<std::mutex> lock(channel->user_list_mutex);
    channel->user_list.clear();
    
    std::istringstream iss(names_str);
    std::string name;
    while (iss >> name) {
        // Remove mode prefixes (@, +, etc.)
        while (!name.empty() && (name[0] == '@' || name[0] == '+' || name[0] == '%' || name[0] == '&' || name[0] == '~')) {
            name = name.substr(1);
        }
        if (!name.empty()) {
            channel->user_list.push_back(name);
        }
    }
    
    if (user_list_callback_) {
        user_list_callback_(channel->channel, channel->user_list);
    }
}

// 处理JOIN/PART消息
void IrcClientManager::handleJoinPart(IrcChannel* channel, const std::string& line, bool is_join) {
    // Format: :nick!user@host JOIN #channel
    // Format: :nick!user@host PART #channel :reason
    size_t colon_pos = line.find(':');
    if (colon_pos == std::string::npos) return;
    
    size_t exclamation = line.find('!', colon_pos);
    if (exclamation == std::string::npos) return;
    
    std::string nickname = line.substr(colon_pos + 1, exclamation - colon_pos - 1);
    
    if (user_join_part_callback_) {
        user_join_part_callback_(channel->channel, nickname, is_join);
    }
    
    // Update user list
    std::lock_guard<std::mutex> lock(channel->user_list_mutex);
    if (is_join) {
        // Add user if not already in list
        auto it = std::find(channel->user_list.begin(), channel->user_list.end(), nickname);
        if (it == channel->user_list.end()) {
            channel->user_list.push_back(nickname);
        }
    } else {
        // Remove user from list
        channel->user_list.erase(
            std::remove(channel->user_list.begin(), channel->user_list.end(), nickname),
            channel->user_list.end()
        );
    }
    
    if (user_list_callback_) {
        user_list_callback_(channel->channel, channel->user_list);
    }
}

// SSL/TLS握手
bool IrcClientManager::performSslHandshake(IrcChannel* channel) {
    // Initialize OpenSSL (thread-safe)
    static std::once_flag ssl_init_flag;
    std::call_once(ssl_init_flag, []() {
        SSL_library_init();
        SSL_load_error_strings();
        OpenSSL_add_all_algorithms();
    });
    
    // Create SSL context
    const SSL_METHOD* method = TLS_client_method();
    SSL_CTX* ctx = SSL_CTX_new(method);
    if (!ctx) {
        V2TIM_LOG(kError, "[IRC] Failed to create SSL context");
        return false;
    }
    
    // Set options
    SSL_CTX_set_options(ctx, SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3);
    
    // Create SSL connection
    SSL* ssl = SSL_new(ctx);
    if (!ssl) {
        V2TIM_LOG(kError, "[IRC] Failed to create SSL connection");
        SSL_CTX_free(ctx);
        return false;
    }
    
    // Set socket
    if (SSL_set_fd(ssl, channel->sock_fd) != 1) {
        V2TIM_LOG(kError, "[IRC] Failed to set SSL file descriptor");
        SSL_free(ssl);
        SSL_CTX_free(ctx);
        return false;
    }
    
    // Perform handshake
    int ret = SSL_connect(ssl);
    if (ret != 1) {
        int err = SSL_get_error(ssl, ret);
        V2TIM_LOG(kError, "[IRC] SSL handshake failed: {}", ERR_error_string(err, nullptr));
        SSL_free(ssl);
        SSL_CTX_free(ctx);
        return false;
    }
    
    // Store SSL context and connection in channel
    channel->ssl_ctx = ctx;
    channel->ssl = ssl;
    
    V2TIM_LOG(kInfo, "[IRC] SSL handshake successful");
    return true;
}

// SSL发送数据
ssize_t IrcClientManager::sslSend(SSL* ssl, const void* buf, size_t len) {
    return SSL_write(ssl, buf, len);
}

// SSL接收数据
ssize_t IrcClientManager::sslRecv(SSL* ssl, void* buf, size_t len) {
    return SSL_read(ssl, buf, len);
}

// 设置连接状态回调
void IrcClientManager::setConnectionStatusCallback(ConnectionStatusCallback callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    connection_status_callback_ = callback;
}

// 设置用户列表回调
void IrcClientManager::setUserListCallback(UserListCallback callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    user_list_callback_ = callback;
}

// 设置用户加入/离开回调
void IrcClientManager::setUserJoinPartCallback(UserJoinPartCallback callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    user_join_part_callback_ = callback;
}

// 设置自定义昵称
void IrcClientManager::setCustomNickname(const std::string& channel, const std::string& nickname) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = channels_.find(channel);
    if (it != channels_.end()) {
        it->second->custom_nickname = nickname;
        it->second->base_nickname = nickname.empty() ? 
            ("ToxUser_" + channel.substr(1, 5)) : nickname;
        std::replace(it->second->base_nickname.begin(), it->second->base_nickname.end(), '#', 'x');
        it->second->nickname_suffix = 0;
    }
}

// 获取用户列表
std::vector<std::string> IrcClientManager::getUserList(const std::string& channel) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = channels_.find(channel);
    if (it != channels_.end()) {
        std::lock_guard<std::mutex> user_lock(it->second->user_list_mutex);
        return it->second->user_list;
    }
    return {};
}

