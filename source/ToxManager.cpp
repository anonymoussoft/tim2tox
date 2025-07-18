// tox_manager.cpp
#include <iostream>
#include "ToxManager.h"
#include "V2TIMLog.h"
#include "toxcore/tox.h"
#include <fstream>
#include <string> // Required for std::to_string
#include <vector> // Required for std::vector

// 单例实例获取
ToxManager& ToxManager::getInstance() {
    static ToxManager instance;
    return instance;
}

// 构造函数
ToxManager::ToxManager() : tox_(nullptr, &toxDeleter) {}

// 自定义删除器实现
void ToxManager::toxDeleter(Tox* tox) {
    tox_kill(tox);
}

// 初始化实现
void ToxManager::initialize(const Tox_Options* options,
                           const uint8_t* savedata,
                           size_t savedata_length) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (tox_) {
        throw std::runtime_error("Tox instance already initialized");
    }

    // Initialize logger (assuming V2TIMLog config happens elsewhere, e.g., in V2TIMManagerImpl::InitSDK)
    // V2TIMLog(kInfo, "ToxManager initializing...");

    // Create Tox options
    Tox_Options default_options;
    tox_options_default(&default_options);
    if (savedata && savedata_length > 0) {
        default_options.savedata_type = TOX_SAVEDATA_TYPE_TOX_SAVE;
        default_options.savedata_data = savedata;
        default_options.savedata_length = savedata_length;
    }

    // Create Tox instance
    TOX_ERR_NEW err_new;
    Tox* tox_instance = tox_new(&default_options, &err_new);
    if (tox_instance == nullptr || err_new != TOX_ERR_NEW_OK) {
        // V2TIMLog(kFatal, "Failed to create Tox instance, error: {}", err_new);
        return;
    }
    
    // 使用reset来设置unique_ptr
    tox_.reset(tox_instance);

    // Store the instance pointer for use in iterate
    // tox_self_set_user_data(tox_.get(), this);
    // Note: We'll pass 'this' directly to tox_iterate instead of using tox_self_set_user_data

    // Bootstrap (Example - replace with actual node)
    // TODO: Move bootstrap logic to V2TIMManagerImpl::Login or a dedicated connection manager
    // const char* bootstrap_node = "tox.ngc.zone";
    // uint16_t port = 33445;
    // uint8_t public_key_bytes[TOX_PUBLIC_KEY_SIZE];
    // TODO: Replace with actual public key hex string and convert
    // const char* public_key_hex = "F404ABAA1C99A9D37D61AB54898F56793E1DEF8BD46B1038B9D82258460A593D";
    // if (tox_hex_to_bytes(public_key_hex, strlen(public_key_hex), public_key_bytes, TOX_PUBLIC_KEY_SIZE)) {
    //     TOX_ERR_BOOTSTRAP err_bootstrap;
    //     tox_bootstrap(tox_, bootstrap_node, port, public_key_bytes, &err_bootstrap);
    //     if (err_bootstrap != TOX_ERR_BOOTSTRAP_OK) {
    //         V2TIMLog(kError, "Bootstrap failed with error: {}", err_bootstrap);
    //     }
    // } else {
    //     V2TIMLog(kError, "Failed to convert bootstrap public key hex string.");
    // }

    // V2TIMLog(kInfo, "ToxManager initialized successfully.");
}

// 关闭实现
void ToxManager::shutdown() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (tox_) {
        // 不需要手动调用tox_kill，unique_ptr会通过toxDeleter自动处理
        tox_.reset(nullptr);
        // V2TIMLog(kInfo, "ToxManager shut down.");
    }
}

// 获取Tox实例
Tox* ToxManager::getTox() const {
    return tox_.get();
}

// 迭代实现
void ToxManager::iterate(uint32_t timeout) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (tox_) {
        tox_iterate(tox_.get(), this);
    }
}

// 回调函数实现
void ToxManager::onSelfConnectionStatus(Tox* tox, TOX_CONNECTION connection_status, void* user_data) {
    ToxManager* manager = static_cast<ToxManager*>(user_data);
    if (manager && manager->self_connection_status_cb_) {
        manager->self_connection_status_cb_(connection_status);
    }
}

void ToxManager::onFriendRequest(Tox* tox, const uint8_t* public_key, const uint8_t* message, size_t length, void* user_data) {
    ToxManager* manager = static_cast<ToxManager*>(user_data);
    if (manager && manager->friend_request_cb_) {
        manager->friend_request_cb_(public_key, message, length);
    }
}

void ToxManager::onFriendMessage(Tox* tox, uint32_t friend_number, TOX_MESSAGE_TYPE type, const uint8_t* message, size_t length, void* user_data) {
    ToxManager* manager = static_cast<ToxManager*>(user_data);
    if (manager && manager->friend_message_cb_) {
        manager->friend_message_cb_(friend_number, type, message, length);
    }
}

void ToxManager::onFriendName(Tox* tox, uint32_t friend_number, const uint8_t* name, size_t length, void* user_data) {
    ToxManager* manager = static_cast<ToxManager*>(user_data);
    if (manager && manager->friend_name_cb_) {
        manager->friend_name_cb_(friend_number, name, length);
    }
}

void ToxManager::onFriendStatusMessage(Tox* tox, uint32_t friend_number, const uint8_t* message, size_t length, void* user_data) {
    ToxManager* manager = static_cast<ToxManager*>(user_data);
    if (manager && manager->friend_status_message_cb_) {
        manager->friend_status_message_cb_(friend_number, message, length);
    }
}

void ToxManager::onFriendStatus(Tox* tox, uint32_t friend_number, TOX_USER_STATUS status, void* user_data) {
    ToxManager* manager = static_cast<ToxManager*>(user_data);
    if (manager && manager->friend_status_cb_) {
        manager->friend_status_cb_(friend_number, status);
    }
}

void ToxManager::onFriendConnectionStatus(Tox* tox, uint32_t friend_number, TOX_CONNECTION connection_status, void* user_data) {
    ToxManager* manager = static_cast<ToxManager*>(user_data);
    if (manager && manager->friend_connection_status_cb_) {
        manager->friend_connection_status_cb_(friend_number, connection_status);
    }
}

void ToxManager::onFriendReadReceipt(Tox* tox, uint32_t friend_number, uint32_t message_id, void* user_data) {
    ToxManager* manager = static_cast<ToxManager*>(user_data);
    if (manager && manager->friend_read_receipt_cb_) {
        manager->friend_read_receipt_cb_(friend_number, message_id);
    }
}

void ToxManager::onFriendTyping(Tox* tox, uint32_t friend_number, bool typing, void* user_data) {
    ToxManager* manager = static_cast<ToxManager*>(user_data);
    if (manager && manager->friend_typing_cb_) {
        manager->friend_typing_cb_(friend_number, typing);
    }
}

void ToxManager::onFileRecv(Tox* tox, uint32_t friend_number, uint32_t file_number, uint32_t kind, uint64_t file_size, const uint8_t* filename, size_t filename_length, void* user_data) {
    ToxManager* manager = static_cast<ToxManager*>(user_data);
    if (manager && manager->file_recv_cb_) {
        manager->file_recv_cb_(friend_number, file_number, kind, file_size, filename, filename_length);
    }
}

void ToxManager::onFileControl(Tox* tox, uint32_t friend_number, uint32_t file_number, TOX_FILE_CONTROL control, void* user_data) {
    ToxManager* manager = static_cast<ToxManager*>(user_data);
    if (manager && manager->file_control_cb_) {
        manager->file_control_cb_(friend_number, file_number, control);
    }
}

void ToxManager::onFileChunkRequest(Tox* tox, uint32_t friend_number, uint32_t file_number, uint64_t position, size_t length, void* user_data) {
    ToxManager* manager = static_cast<ToxManager*>(user_data);
    if (manager && manager->file_chunk_request_cb_) {
        manager->file_chunk_request_cb_(friend_number, file_number, position, length);
    }
}

void ToxManager::onFileRecvChunk(Tox* tox, uint32_t friend_number, uint32_t file_number, uint64_t position, const uint8_t* data, size_t length, void* user_data) {
    ToxManager* manager = static_cast<ToxManager*>(user_data);
    if (manager && manager->file_recv_chunk_cb_) {
        manager->file_recv_chunk_cb_(friend_number, file_number, position, data, length);
    }
}

void ToxManager::onGroupInvite(Tox* tox, uint32_t friend_number, TOX_CONFERENCE_TYPE type, const uint8_t* cookie, size_t length, void* user_data) {
    ToxManager* manager = static_cast<ToxManager*>(user_data);
    if (manager && manager->group_invite_cb_) {
        manager->group_invite_cb_(friend_number, type, cookie, length);
    }
}

void ToxManager::onGroupMessage(Tox* tox, uint32_t conference_number, uint32_t peer_number, TOX_MESSAGE_TYPE type, const uint8_t* message, size_t length, void* user_data) {
    ToxManager* manager = static_cast<ToxManager*>(user_data);
    if (manager && manager->group_message_cb_) {
        manager->group_message_cb_(conference_number, peer_number, type, message, length);
    }
}

void ToxManager::onGroupTitle(Tox* tox, uint32_t conference_number, uint32_t peer_number, const uint8_t* title, size_t length, void* user_data) {
    ToxManager* manager = static_cast<ToxManager*>(user_data);
    if (manager && manager->group_title_cb_) {
        manager->group_title_cb_(conference_number, peer_number, title, length);
    }
}

void ToxManager::onGroupPeerName(Tox* tox, uint32_t conference_number, uint32_t peer_number, const uint8_t* name, size_t length, void* user_data) {
    ToxManager* manager = static_cast<ToxManager*>(user_data);
    if (manager && manager->group_peer_name_cb_) {
        manager->group_peer_name_cb_(conference_number, peer_number, name, length);
    }
}

void ToxManager::onGroupPeerListChanged(Tox* tox, uint32_t conference_number, void* user_data) {
    ToxManager* manager = static_cast<ToxManager*>(user_data);
    if (manager && manager->group_peer_list_changed_cb_) {
        manager->group_peer_list_changed_cb_(conference_number);
    }
}

void ToxManager::onFriendLossyPacket(Tox* tox, uint32_t friend_number, const uint8_t* data, size_t length, void* user_data) {
    ToxManager* manager = static_cast<ToxManager*>(user_data);
    if (manager && manager->friend_lossy_packet_cb_) {
        manager->friend_lossy_packet_cb_(friend_number, data, length);
    }
}

void ToxManager::onFriendLosslessPacket(Tox* tox, uint32_t friend_number, const uint8_t* data, size_t length, void* user_data) {
    ToxManager* manager = static_cast<ToxManager*>(user_data);
    if (manager && manager->friend_lossless_packet_cb_) {
        manager->friend_lossless_packet_cb_(friend_number, data, length);
    }
}

// 更新回调设置方法
void ToxManager::setSelfConnectionStatusCallback(SelfConnectionStatusCallback cb) {
    std::lock_guard<std::mutex> lock(mutex_);
    self_connection_status_cb_ = std::move(cb);
    tox_callback_self_connection_status(tox_.get(), onSelfConnectionStatus);
}

void ToxManager::setFriendRequestCallback(FriendRequestCallback cb) {
    std::lock_guard<std::mutex> lock(mutex_);
    friend_request_cb_ = std::move(cb);
    tox_callback_friend_request(tox_.get(), onFriendRequest);
}

void ToxManager::setFriendMessageCallback(FriendMessageCallback cb) {
    std::lock_guard<std::mutex> lock(mutex_);
    friend_message_cb_ = std::move(cb);
    tox_callback_friend_message(tox_.get(), onFriendMessage);
}

void ToxManager::setFriendNameCallback(FriendNameCallback cb) {
    std::lock_guard<std::mutex> lock(mutex_);
    friend_name_cb_ = std::move(cb);
    tox_callback_friend_name(tox_.get(), onFriendName);
}

void ToxManager::setFriendStatusMessageCallback(FriendStatusMessageCallback cb) {
    std::lock_guard<std::mutex> lock(mutex_);
    friend_status_message_cb_ = std::move(cb);
    tox_callback_friend_status_message(tox_.get(), onFriendStatusMessage);
}

void ToxManager::setFriendStatusCallback(FriendStatusCallback cb) {
    std::lock_guard<std::mutex> lock(mutex_);
    friend_status_cb_ = std::move(cb);
    tox_callback_friend_status(tox_.get(), onFriendStatus);
}

void ToxManager::setFriendConnectionStatusCallback(FriendConnectionStatusCallback cb) {
    std::lock_guard<std::mutex> lock(mutex_);
    friend_connection_status_cb_ = std::move(cb);
    tox_callback_friend_connection_status(tox_.get(), onFriendConnectionStatus);
}

void ToxManager::setFriendReadReceiptCallback(FriendReadReceiptCallback cb) {
    std::lock_guard<std::mutex> lock(mutex_);
    friend_read_receipt_cb_ = std::move(cb);
    tox_callback_friend_read_receipt(tox_.get(), onFriendReadReceipt);
}

void ToxManager::setFriendTypingCallback(FriendTypingCallback cb) {
    std::lock_guard<std::mutex> lock(mutex_);
    friend_typing_cb_ = std::move(cb);
    tox_callback_friend_typing(tox_.get(), onFriendTyping);
}

void ToxManager::setFileRecvCallback(FileRecvCallback cb) {
    std::lock_guard<std::mutex> lock(mutex_);
    file_recv_cb_ = std::move(cb);
    tox_callback_file_recv(tox_.get(), onFileRecv);
}

void ToxManager::setFileControlCallback(FileControlCallback cb) {
    std::lock_guard<std::mutex> lock(mutex_);
    file_control_cb_ = std::move(cb);
    tox_callback_file_recv_control(tox_.get(), onFileControl);
}

void ToxManager::setFileChunkRequestCallback(FileChunkRequestCallback cb) {
    std::lock_guard<std::mutex> lock(mutex_);
    file_chunk_request_cb_ = std::move(cb);
    tox_callback_file_chunk_request(tox_.get(), onFileChunkRequest);
}

void ToxManager::setFileRecvChunkCallback(FileRecvChunkCallback cb) {
    std::lock_guard<std::mutex> lock(mutex_);
    file_recv_chunk_cb_ = std::move(cb);
    // Registration moved to initialize
    // tox_callback_file_recv_chunk(tox_.get(), onFileRecvChunk);
}

void ToxManager::setGroupInviteCallback(GroupInviteCallback cb) {
    std::lock_guard<std::mutex> lock(mutex_);
    group_invite_cb_ = std::move(cb);
    // Registration moved to initialize
    // tox_callback_conference_invite(tox_.get(), onGroupInvite);
}

void ToxManager::setGroupMessageCallback(GroupMessageCallback cb) {
    std::lock_guard<std::mutex> lock(mutex_);
    group_message_cb_ = std::move(cb);
    // Registration moved to initialize
    // tox_callback_conference_message(tox_.get(), onGroupMessage);
}

void ToxManager::setGroupTitleCallback(GroupTitleCallback cb) {
    std::lock_guard<std::mutex> lock(mutex_);
    group_title_cb_ = std::move(cb);
    // Registration moved to initialize
    // tox_callback_conference_title(tox_.get(), onGroupTitle);
}

void ToxManager::setGroupPeerNameCallback(GroupPeerNameCallback cb) {
    std::lock_guard<std::mutex> lock(mutex_);
    group_peer_name_cb_ = std::move(cb);
    // Registration moved to initialize
    // tox_callback_conference_peer_name(tox_.get(), onGroupPeerName);
}

void ToxManager::setGroupPeerListChangedCallback(GroupPeerListChangedCallback cb) {
    std::lock_guard<std::mutex> lock(mutex_);
    group_peer_list_changed_cb_ = std::move(cb);
    // Registration moved to initialize
    // tox_callback_conference_peer_list_changed(tox_.get(), onGroupPeerListChanged);
}

void ToxManager::setFriendLossyPacketCallback(FriendLossyPacketCallback cb) {
    std::lock_guard<std::mutex> lock(mutex_);
    friend_lossy_packet_cb_ = std::move(cb);
    // Registration moved to initialize
    // tox_callback_friend_lossy_packet(tox_.get(), onFriendLossyPacket);
}

void ToxManager::setFriendLosslessPacketCallback(FriendLosslessPacketCallback cb) {
    std::lock_guard<std::mutex> lock(mutex_);
    friend_lossless_packet_cb_ = std::move(cb);
    tox_callback_friend_lossless_packet(tox_.get(), onFriendLosslessPacket);
}

// 用户信息相关实现
bool ToxManager::setName(const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!tox_) return false;
    TOX_ERR_SET_INFO error;
    return tox_self_set_name(tox_.get(), 
        reinterpret_cast<const uint8_t*>(name.c_str()), 
        name.length(), &error) && error == TOX_ERR_SET_INFO_OK;
}

std::string ToxManager::getName() const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!tox_) return "";
    size_t length = tox_self_get_name_size(tox_.get());
    std::vector<uint8_t> name(length);
    tox_self_get_name(tox_.get(), name.data());
    return std::string(reinterpret_cast<char*>(name.data()), length);
}

bool ToxManager::setStatusMessage(const std::string& message) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!tox_) return false;
    TOX_ERR_SET_INFO error;
    return tox_self_set_status_message(tox_.get(),
        reinterpret_cast<const uint8_t*>(message.c_str()),
        message.length(), &error) && error == TOX_ERR_SET_INFO_OK;
}

std::string ToxManager::getStatusMessage() const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!tox_) return "";
    size_t length = tox_self_get_status_message_size(tox_.get());
    std::vector<uint8_t> message(length);
    tox_self_get_status_message(tox_.get(), message.data());
    return std::string(reinterpret_cast<char*>(message.data()), length);
}

bool ToxManager::setStatus(TOX_USER_STATUS status) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!tox_) return false;
    tox_self_set_status(tox_.get(), status);
    return true;
}

TOX_USER_STATUS ToxManager::getStatus() const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!tox_) return TOX_USER_STATUS_NONE;
    return tox_self_get_status(tox_.get());
}

std::string ToxManager::getAddress() const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!tox_) return "";
    std::vector<uint8_t> address(TOX_ADDRESS_SIZE);
    tox_self_get_address(tox_.get(), address.data());
    // Convert binary address to hexadecimal string
    std::string hex_address;
    hex_address.reserve(TOX_ADDRESS_SIZE * 2);
    for (uint8_t byte : address) {
        char hex[3];
        snprintf(hex, sizeof(hex), "%02X", byte);
        hex_address += hex;
    }
    return hex_address;
}

// 好友相关实现
uint32_t ToxManager::addFriend(const uint8_t* address, const uint8_t* message,
                              size_t length, TOX_ERR_FRIEND_ADD* error) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!tox_) {
        if (error) *error = TOX_ERR_FRIEND_ADD_NULL;
        return UINT32_MAX;
    }
    return tox_friend_add(tox_.get(), address, message, length, error);
}

bool ToxManager::deleteFriend(uint32_t friend_number, TOX_ERR_FRIEND_DELETE* error) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!tox_) {
        if (error) *error = TOX_ERR_FRIEND_DELETE_FRIEND_NOT_FOUND;
        return false;
    }
    return tox_friend_delete(tox_.get(), friend_number, error);
}

bool ToxManager::sendMessage(uint32_t friend_number, TOX_MESSAGE_TYPE type,
                           const uint8_t* message, size_t length, uint32_t* message_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!tox_) return false;
    TOX_ERR_FRIEND_SEND_MESSAGE error;
    uint32_t res = tox_friend_send_message(tox_.get(), friend_number, type,
                                         message, length, &error);
    if (error != TOX_ERR_FRIEND_SEND_MESSAGE_OK) return false;
    if (message_id) *message_id = res;
    return true;
}

bool ToxManager::getFriendPublicKey(uint32_t friend_number, uint8_t* public_key) const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!tox_) return false;
    TOX_ERR_FRIEND_GET_PUBLIC_KEY error;
    return tox_friend_get_public_key(tox_.get(), friend_number, public_key, &error) &&
           error == TOX_ERR_FRIEND_GET_PUBLIC_KEY_OK;
}

std::string ToxManager::getFriendName(uint32_t friend_number) const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!tox_) return "";
    TOX_ERR_FRIEND_QUERY error;
    size_t length = tox_friend_get_name_size(tox_.get(), friend_number, &error);
    if (error != TOX_ERR_FRIEND_QUERY_OK) return "";
    std::vector<uint8_t> name(length);
    if (!tox_friend_get_name(tox_.get(), friend_number, name.data(), &error) ||
        error != TOX_ERR_FRIEND_QUERY_OK) return "";
    return std::string(reinterpret_cast<char*>(name.data()), length);
}

std::string ToxManager::getFriendStatusMessage(uint32_t friend_number) const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!tox_) return "";
    TOX_ERR_FRIEND_QUERY error;
    size_t length = tox_friend_get_status_message_size(tox_.get(), friend_number, &error);
    if (error != TOX_ERR_FRIEND_QUERY_OK) return "";
    std::vector<uint8_t> message(length);
    if (!tox_friend_get_status_message(tox_.get(), friend_number, message.data(), &error) ||
        error != TOX_ERR_FRIEND_QUERY_OK) return "";
    return std::string(reinterpret_cast<char*>(message.data()), length);
}

TOX_CONNECTION ToxManager::getFriendConnectionStatus(uint32_t friend_number) const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!tox_) return TOX_CONNECTION_NONE;
    TOX_ERR_FRIEND_QUERY error;
    return tox_friend_get_connection_status(tox_.get(), friend_number, &error);
}

TOX_USER_STATUS ToxManager::getFriendStatus(uint32_t friend_number) const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!tox_) return TOX_USER_STATUS_NONE;
    TOX_ERR_FRIEND_QUERY error;
    return tox_friend_get_status(tox_.get(), friend_number, &error);
}

bool ToxManager::getFriendLastOnline(uint32_t friend_number, time_t* last_online) const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!tox_ || !last_online) return false;
    TOX_ERR_FRIEND_GET_LAST_ONLINE error;
    *last_online = tox_friend_get_last_online(tox_.get(), friend_number, &error);
    return error == TOX_ERR_FRIEND_GET_LAST_ONLINE_OK;
}

// 文件传输相关实现
uint32_t ToxManager::sendFile(uint32_t friend_number, uint32_t kind,
                            uint64_t file_size, const uint8_t* file_id,
                            const uint8_t* filename, size_t filename_length,
                            TOX_ERR_FILE_SEND* error) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!tox_) {
        if (error) *error = TOX_ERR_FILE_SEND_NULL;
        return UINT32_MAX;
    }
    return tox_file_send(tox_.get(), friend_number, kind, file_size,
                        file_id, filename, filename_length, error);
}

bool ToxManager::sendFileChunk(uint32_t friend_number, uint32_t file_number,
                             uint64_t position, const uint8_t* data, size_t length,
                             TOX_ERR_FILE_SEND_CHUNK* error) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!tox_) {
        if (error) *error = TOX_ERR_FILE_SEND_CHUNK_NULL;
        return false;
    }
    return tox_file_send_chunk(tox_.get(), friend_number, file_number,
                              position, data, length, error);
}

bool ToxManager::fileControl(uint32_t friend_number, uint32_t file_number,
                           TOX_FILE_CONTROL control, TOX_ERR_FILE_CONTROL* error) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!tox_) {
        if (error) *error = TOX_ERR_FILE_CONTROL_FRIEND_NOT_FOUND;
        return false;
    }
    return tox_file_control(tox_.get(), friend_number, file_number, control, error);
}

// 群组相关实现
uint32_t ToxManager::createGroup(TOX_ERR_CONFERENCE_NEW* error) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!tox_) {
        if (error) *error = TOX_ERR_CONFERENCE_NEW_INIT;
        return UINT32_MAX;
    }
    return tox_conference_new(tox_.get(), error);
}

bool ToxManager::deleteGroup(uint32_t group_number, TOX_ERR_CONFERENCE_DELETE* error) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!tox_) {
        if (error) *error = TOX_ERR_CONFERENCE_DELETE_CONFERENCE_NOT_FOUND;
        return false;
    }
    return tox_conference_delete(tox_.get(), group_number, error);
}

uint32_t ToxManager::joinGroup(uint32_t friend_number, const uint8_t* cookie,
                             size_t length, TOX_ERR_CONFERENCE_JOIN* error) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!tox_) {
        if (error) *error = TOX_ERR_CONFERENCE_JOIN_INIT_FAIL;
        return UINT32_MAX;
    }
    return tox_conference_join(tox_.get(), friend_number, cookie, length, error);
}

bool ToxManager::groupSendMessage(uint32_t group_number, TOX_MESSAGE_TYPE type,
                                const uint8_t* message, size_t length,
                                TOX_ERR_CONFERENCE_SEND_MESSAGE* error) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!tox_) {
        if (error) *error = TOX_ERR_CONFERENCE_SEND_MESSAGE_NO_CONNECTION;
        return false;
    }
    return tox_conference_send_message(tox_.get(), group_number, type, message, length, error);
}

bool ToxManager::setGroupTitle(uint32_t group_number, const uint8_t* title,
                             size_t length, TOX_ERR_CONFERENCE_TITLE* error) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!tox_) {
        if (error) *error = TOX_ERR_CONFERENCE_TITLE_CONFERENCE_NOT_FOUND;
        return false;
    }
    return tox_conference_set_title(tox_.get(), group_number, title, length, error);
}

size_t ToxManager::getGroupTitle(uint32_t group_number, uint8_t* title) const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!tox_) return 0;
    TOX_ERR_CONFERENCE_TITLE error;
    return tox_conference_get_title_size(tox_.get(), group_number, &error);
}

size_t ToxManager::getGroupPeerName(uint32_t group_number, uint32_t peer_number,
                                  uint8_t* name) const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!tox_) return 0;
    TOX_ERR_CONFERENCE_PEER_QUERY error;
    return tox_conference_peer_get_name_size(tox_.get(), group_number, peer_number, &error);
}

uint32_t ToxManager::getGroupPeerCount(uint32_t group_number) const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!tox_) return 0;
    TOX_ERR_CONFERENCE_PEER_QUERY error;
    return tox_conference_peer_count(tox_.get(), group_number, &error);
}

// 数据保存和加载实现
std::vector<uint8_t> ToxManager::getSaveData() const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!tox_) return {};
    
    size_t size = tox_get_savedata_size(tox_.get());
    std::vector<uint8_t> data(size);
    tox_get_savedata(tox_.get(), data.data());
    return data;
}

bool ToxManager::saveTo(const std::string& path) const {
    try {
        auto data = getSaveData();
        if (data.empty()) return false;

        std::ofstream file(path, std::ios::binary);
        if (!file) return false;

        file.write(reinterpret_cast<const char*>(data.data()), data.size());
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

bool ToxManager::loadFrom(const std::string& path) {
    try {
        std::ifstream file(path, std::ios::binary | std::ios::ate);
        if (!file) return false;

        auto size = file.tellg();
        if (size <= 0) return false;

        std::vector<uint8_t> data(size);
        file.seekg(0);
        file.read(reinterpret_cast<char*>(data.data()), size);

        // 创建新的 Tox 选项
        Tox_Options options;
        tox_options_default(&options);
        options.savedata_type = TOX_SAVEDATA_TYPE_TOX_SAVE;
        options.savedata_data = data.data();
        options.savedata_length = data.size();

        // 初始化 Tox
        initialize(&options, data.data(), data.size());
        return true;
    } catch (const std::exception&) {
        return false;
    }
}