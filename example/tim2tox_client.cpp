#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <memory>
#include <sodium/utils.h>

// Include tim2tox headers
#include "../include/V2TIMManager.h"
#include "../include/V2TIMMessage.h"
#include "../include/V2TIMFriendship.h"
#include "../include/V2TIMCallback.h"
#include "../include/V2TIMCommon.h"
#include "../third_party/c-toxcore/toxcore/tox.h"

class Tim2ToxClient : public V2TIMSDKListener {
private:
    V2TIMManager* manager_;
    std::string server_tox_id_;
    uint32_t friend_number_;
    bool connected_;
    bool friend_connected_;

public:
    Tim2ToxClient() : connected_(false), friend_connected_(false), friend_number_(0) {
        manager_ = V2TIMManager::GetInstance();
    }

    ~Tim2ToxClient() {
        if (manager_) {
            manager_->UnInitSDK();
        }
    }

    bool Initialize() {
        // Initialize SDK
        V2TIMSDKConfig config;
        config.logLevel = V2TIM_LOG_DEBUG;
        
        if (!manager_->InitSDK(123456, config)) {
            std::cerr << "Failed to initialize SDK" << std::endl;
            return false;
        }

        // Register callbacks
        manager_->AddSDKListener(this);
        
        std::cout << "Tim2Tox client initialized successfully" << std::endl;
        return true;
    }

    void SetServerToxID(const std::string& tox_id) {
        server_tox_id_ = tox_id;
        std::cout << "Server Tox ID set to: " << tox_id << std::endl;
    }

    bool ConnectToServer() {
        if (server_tox_id_.empty()) {
            std::cerr << "Server Tox ID not set" << std::endl;
            return false;
        }

        // Convert hex string to binary
        uint8_t tox_id_bin[TOX_ADDRESS_SIZE];
        if (sodium_hex2bin(tox_id_bin, sizeof(tox_id_bin), 
                          server_tox_id_.c_str(), server_tox_id_.length(),
                          NULL, NULL, NULL) != 0) {
            std::cerr << "Invalid Tox ID format" << std::endl;
            return false;
        }

        // Add friend (this will send friend request)
        std::string message = "Hello from Tim2Tox client!";
        // Note: This is a simplified version. In real implementation, you'd need to convert tox_id_bin to V2TIMString
        std::cout << "Friend request would be sent here" << std::endl;
        return true;

        std::cout << "Friend request sent to server" << std::endl;
        return true;
    }

    bool SendMessage(const std::string& message) {
        if (!friend_connected_) {
            std::cerr << "Not connected to server" << std::endl;
            return false;
        }

        // Note: This is a simplified version. In real implementation, you'd use the proper message sending API
        std::cout << "Message would be sent: " << message << std::endl;
        return true;

        std::cout << "Message sent: " << message << std::endl;
        return true;
    }

    void Run() {
        std::cout << "Tim2Tox client running. Type 'quit' to exit." << std::endl;
        
        std::string input;
        while (std::getline(std::cin, input)) {
            if (input == "quit" || input == "exit") {
                break;
            }
            
            if (input == "connect") {
                ConnectToServer();
            } else if (input.substr(0, 4) == "send") {
                std::string message = input.substr(5); // Remove "send "
                SendMessage(message);
            } else if (input == "status") {
                PrintStatus();
            } else if (input == "help") {
                PrintHelp();
            } else if (!input.empty()) {
                SendMessage(input);
            }
        }
    }

    void PrintStatus() {
        std::cout << "=== Client Status ===" << std::endl;
        std::cout << "Connected: " << (connected_ ? "Yes" : "No") << std::endl;
        std::cout << "Friend connected: " << (friend_connected_ ? "Yes" : "No") << std::endl;
        std::cout << "Server Tox ID: " << server_tox_id_ << std::endl;
        std::cout << "===================" << std::endl;
    }

    void PrintHelp() {
        std::cout << "=== Commands ===" << std::endl;
        std::cout << "connect - Connect to server" << std::endl;
        std::cout << "send <message> - Send message to server" << std::endl;
        std::cout << "status - Show connection status" << std::endl;
        std::cout << "help - Show this help" << std::endl;
        std::cout << "quit/exit - Exit client" << std::endl;
        std::cout << "=================" << std::endl;
    }

    // V2TIMSDKListener callbacks
    void OnConnecting() {
        std::cout << "Connecting to Tox network..." << std::endl;
    }

    void OnConnectSuccess() {
        connected_ = true;
        std::cout << "Connected to Tox network successfully!" << std::endl;
    }

    void OnConnectFailed(int error_code, const V2TIMString& error_msg) {
        connected_ = false;
        std::cerr << "Failed to connect to Tox network: " << error_msg.CString() << std::endl;
    }

    void OnKickedOffline() {
        connected_ = false;
        friend_connected_ = false;
        std::cout << "Kicked offline" << std::endl;
    }

    void OnUserSigExpired() {
        std::cout << "User signature expired" << std::endl;
    }

    // Friendship callbacks
    void OnFriendApplicationListAdded(const V2TIMFriendApplication& application) {
        std::cout << "Friend application received from: " << application.userID.CString() << std::endl;
    }

    void OnFriendApplicationListDeleted(const V2TIMString& userID) {
        std::cout << "Friend application deleted for: " << userID.CString() << std::endl;
    }

    void OnFriendApplicationListRead() {
        std::cout << "Friend application list read" << std::endl;
    }

    void OnFriendListAdded(const V2TIMFriendInfo& info) {
        std::cout << "Friend added: " << info.userID.CString() << std::endl;
        friend_number_ = 0; // In real implementation, you'd get the friend number
        friend_connected_ = true;
    }

    void OnFriendListDeleted(const V2TIMString& userID) {
        std::cout << "Friend deleted: " << userID.CString() << std::endl;
        friend_connected_ = false;
    }

    void OnBlackListAdd(const V2TIMFriendInfo& info) {
        std::cout << "User added to blacklist: " << info.userID.CString() << std::endl;
    }

    void OnBlackListDeleted(const V2TIMString& userID) {
        std::cout << "User removed from blacklist: " << userID.CString() << std::endl;
    }

    // Message callbacks
    void OnRecvNewMessage(const V2TIMMessage& message) {
        if (message.elemList.Size() > 0) {
            V2TIMElem* elem = message.elemList[0];
            if (elem->elemType == V2TIM_ELEM_TYPE_TEXT) {
                V2TIMTextElem* textElem = static_cast<V2TIMTextElem*>(elem);
                std::cout << "Received message: " << textElem->text.CString() << std::endl;
            }
        }
    }

    void OnRecvMessageRevoked(const V2TIMString& msgID) {
        std::cout << "Message revoked: " << msgID.CString() << std::endl;
    }

    void OnRecvC2CReadReceipt(const std::vector<V2TIMMessageReceipt>& receiptList) {
        for (const auto& receipt : receiptList) {
            std::cout << "C2C read receipt: " << receipt.msgID.CString() << std::endl;
        }
    }

    void OnRecvMessageReadReceipts(const std::vector<V2TIMMessageReceipt>& receiptList) {
        for (const auto& receipt : receiptList) {
            std::cout << "Group read receipt: " << receipt.msgID.CString() << std::endl;
        }
    }

    void OnMessageModified(const V2TIMMessage& message) {
        std::cout << "Message modified: " << message.msgID.CString() << std::endl;
    }
};

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <server_tox_id>" << std::endl;
        std::cout << "Example: " << argv[0] << " F404ABAA1C99A9D37D61AB54898F56793E1DEF8BD46B1038B9D822E8460FAB67" << std::endl;
        return 1;
    }

    Tim2ToxClient client;
    
    if (!client.Initialize()) {
        std::cerr << "Failed to initialize client" << std::endl;
        return 1;
    }

    client.SetServerToxID(argv[1]);
    client.PrintHelp();

    // Start client
    client.Run();

    return 0;
} 