// tox_manager.h
#pragma once

#include <memory>
#include <mutex>
#include "toxcore/tox.h"
#include <functional>
#include <stdexcept>
#include <vector>

class ToxManager {
public:
    // 删除拷贝构造函数和赋值运算符
    ToxManager(const ToxManager&) = delete;
    ToxManager& operator=(const ToxManager&) = delete;

    // 获取单例实例
    static ToxManager& getInstance();

    // 初始化/关闭方法
    void initialize(const Tox_Options* options = nullptr,
                   const uint8_t* savedata = nullptr,
                   size_t savedata_length = 0);
    void shutdown();

    // 核心功能接口
    Tox* getTox() const;
    void iterate(uint32_t timeout = 0);

    // 数据保存和加载
    std::vector<uint8_t> getSaveData() const;
    bool saveTo(const std::string& path) const;
    bool loadFrom(const std::string& path);

    // 基本功能接口
    // 用户信息相关
    bool setName(const std::string& name);
    std::string getName() const;
    bool setStatusMessage(const std::string& message);
    std::string getStatusMessage() const;
    bool setStatus(TOX_USER_STATUS status);
    TOX_USER_STATUS getStatus() const;
    std::string getAddress() const;

    // 好友相关
    uint32_t addFriend(const uint8_t* address, const uint8_t* message, size_t length, TOX_ERR_FRIEND_ADD* error = nullptr);
    bool deleteFriend(uint32_t friend_number, TOX_ERR_FRIEND_DELETE* error = nullptr);
    bool sendMessage(uint32_t friend_number, TOX_MESSAGE_TYPE type, const uint8_t* message, size_t length, uint32_t* message_id = nullptr);
    bool getFriendPublicKey(uint32_t friend_number, uint8_t* public_key) const;
    std::string getFriendName(uint32_t friend_number) const;
    std::string getFriendStatusMessage(uint32_t friend_number) const;
    TOX_CONNECTION getFriendConnectionStatus(uint32_t friend_number) const;
    TOX_USER_STATUS getFriendStatus(uint32_t friend_number) const;
    bool getFriendLastOnline(uint32_t friend_number, time_t* last_online) const;

    // 文件传输相关
    uint32_t sendFile(uint32_t friend_number, uint32_t kind, uint64_t file_size,
                     const uint8_t* file_id, const uint8_t* filename, size_t filename_length,
                     TOX_ERR_FILE_SEND* error = nullptr);
    bool sendFileChunk(uint32_t friend_number, uint32_t file_number,
                      uint64_t position, const uint8_t* data, size_t length,
                      TOX_ERR_FILE_SEND_CHUNK* error = nullptr);
    bool fileControl(uint32_t friend_number, uint32_t file_number,
                    TOX_FILE_CONTROL control, TOX_ERR_FILE_CONTROL* error = nullptr);
    bool fileSeek(uint32_t friend_number, uint32_t file_number,
                 uint64_t position, TOX_ERR_FILE_SEEK* error = nullptr);

    // 群组相关
    uint32_t createGroup(TOX_ERR_CONFERENCE_NEW* error = nullptr);
    bool deleteGroup(uint32_t group_number, TOX_ERR_CONFERENCE_DELETE* error = nullptr);
    uint32_t joinGroup(uint32_t friend_number, const uint8_t* cookie, size_t length,
                      TOX_ERR_CONFERENCE_JOIN* error = nullptr);
    bool groupSendMessage(uint32_t group_number, TOX_MESSAGE_TYPE type,
                         const uint8_t* message, size_t length,
                         TOX_ERR_CONFERENCE_SEND_MESSAGE* error = nullptr);
    bool setGroupTitle(uint32_t group_number, const uint8_t* title,
                      size_t length, TOX_ERR_CONFERENCE_TITLE* error = nullptr);
    size_t getGroupTitle(uint32_t group_number, uint8_t* title) const;
    size_t getGroupPeerName(uint32_t group_number, uint32_t peer_number,
                           uint8_t* name) const;
    uint32_t getGroupPeerCount(uint32_t group_number) const;

    // 回调类型定义
    using SelfConnectionStatusCallback = std::function<void(TOX_CONNECTION)>;
    using FriendRequestCallback = std::function<void(const uint8_t*, const uint8_t*, size_t)>;
    using FriendMessageCallback = std::function<void(uint32_t, TOX_MESSAGE_TYPE, const uint8_t*, size_t)>;
    using FriendNameCallback = std::function<void(uint32_t, const uint8_t*, size_t)>;
    using FriendStatusMessageCallback = std::function<void(uint32_t, const uint8_t*, size_t)>;
    using FriendStatusCallback = std::function<void(uint32_t, TOX_USER_STATUS)>;
    using FriendConnectionStatusCallback = std::function<void(uint32_t, TOX_CONNECTION)>;
    using FriendReadReceiptCallback = std::function<void(uint32_t, uint32_t)>;
    using FriendTypingCallback = std::function<void(uint32_t, bool)>;
    using FileRecvCallback = std::function<void(uint32_t, uint32_t, uint32_t, uint64_t, const uint8_t*, size_t)>;
    using FileControlCallback = std::function<void(uint32_t friend_number, uint32_t file_number, TOX_FILE_CONTROL control)>;
    using FileChunkRequestCallback = std::function<void(uint32_t, uint32_t, uint64_t, size_t)>;
    using FileRecvChunkCallback = std::function<void(uint32_t, uint32_t, uint64_t, const uint8_t*, size_t)>;
    using GroupInviteCallback = std::function<void(uint32_t, TOX_CONFERENCE_TYPE, const uint8_t*, size_t)>;
    using GroupMessageCallback = std::function<void(uint32_t, uint32_t, TOX_MESSAGE_TYPE, const uint8_t*, size_t)>;
    using GroupTitleCallback = std::function<void(uint32_t, uint32_t, const uint8_t*, size_t)>;
    using GroupPeerNameCallback = std::function<void(uint32_t, uint32_t, const uint8_t*, size_t)>;
    using GroupPeerListChangedCallback = std::function<void(uint32_t)>;
    using FriendLossyPacketCallback = std::function<void(uint32_t, const uint8_t*, size_t)>;
    using FriendLosslessPacketCallback = std::function<void(uint32_t, const uint8_t*, size_t)>;
    using AudioReceiveCallback = std::function<void(uint32_t, const int16_t*, size_t, uint8_t, uint32_t)>;

    // 设置回调的方法
    void setSelfConnectionStatusCallback(SelfConnectionStatusCallback cb);
    void setFriendRequestCallback(FriendRequestCallback cb);
    void setFriendMessageCallback(FriendMessageCallback cb);
    void setFriendNameCallback(FriendNameCallback cb);
    void setFriendStatusMessageCallback(FriendStatusMessageCallback cb);
    void setFriendStatusCallback(FriendStatusCallback cb);
    void setFriendConnectionStatusCallback(FriendConnectionStatusCallback cb);
    void setFriendReadReceiptCallback(FriendReadReceiptCallback cb);
    void setFriendTypingCallback(FriendTypingCallback cb);
    void setFileRecvCallback(FileRecvCallback cb);
    void setFileControlCallback(FileControlCallback cb);
    FileControlCallback getFileControlCallback() const;
    void setFileChunkRequestCallback(FileChunkRequestCallback cb);
    void setFileRecvChunkCallback(FileRecvChunkCallback cb);
    void setGroupInviteCallback(GroupInviteCallback cb);
    void setGroupMessageCallback(GroupMessageCallback cb);
    void setGroupTitleCallback(GroupTitleCallback cb);
    void setGroupPeerNameCallback(GroupPeerNameCallback cb);
    void setGroupPeerListChangedCallback(GroupPeerListChangedCallback cb);
    void setFriendLossyPacketCallback(FriendLossyPacketCallback cb);
    void setFriendLosslessPacketCallback(FriendLosslessPacketCallback cb);
    void setAudioReceiveCallback(AudioReceiveCallback cb);

    // 静态回调函数声明
    static void onSelfConnectionStatus(Tox* tox, TOX_CONNECTION connection_status, void* user_data);
    static void onFriendRequest(Tox* tox, const uint8_t* public_key, const uint8_t* message, size_t length, void* user_data);
    static void onFriendMessage(Tox* tox, uint32_t friend_number, TOX_MESSAGE_TYPE type, const uint8_t* message, size_t length, void* user_data);
    static void onFriendName(Tox* tox, uint32_t friend_number, const uint8_t* name, size_t length, void* user_data);
    static void onFriendStatusMessage(Tox* tox, uint32_t friend_number, const uint8_t* message, size_t length, void* user_data);
    static void onFriendStatus(Tox* tox, uint32_t friend_number, TOX_USER_STATUS status, void* user_data);
    static void onFriendConnectionStatus(Tox* tox, uint32_t friend_number, TOX_CONNECTION connection_status, void* user_data);
    static void onFriendReadReceipt(Tox* tox, uint32_t friend_number, uint32_t message_id, void* user_data);
    static void onFriendTyping(Tox* tox, uint32_t friend_number, bool typing, void* user_data);
    static void onFileRecv(Tox* tox, uint32_t friend_number, uint32_t file_number, uint32_t kind, uint64_t file_size, const uint8_t* filename, size_t filename_length, void* user_data);
    static void onFileControl(Tox* tox, uint32_t friend_number, uint32_t file_number, TOX_FILE_CONTROL control, void* user_data);
    static void onFileChunkRequest(Tox* tox, uint32_t friend_number, uint32_t file_number, uint64_t position, size_t length, void* user_data);
    static void onFileRecvChunk(Tox* tox, uint32_t friend_number, uint32_t file_number, uint64_t position, const uint8_t* data, size_t length, void* user_data);
    static void onGroupInvite(Tox* tox, uint32_t friend_number, TOX_CONFERENCE_TYPE type, const uint8_t* cookie, size_t length, void* user_data);
    static void onGroupMessage(Tox* tox, uint32_t conference_number, uint32_t peer_number, TOX_MESSAGE_TYPE type, const uint8_t* message, size_t length, void* user_data);
    static void onGroupTitle(Tox* tox, uint32_t conference_number, uint32_t peer_number, const uint8_t* title, size_t length, void* user_data);
    static void onGroupPeerName(Tox* tox, uint32_t conference_number, uint32_t peer_number, const uint8_t* name, size_t length, void* user_data);
    static void onGroupPeerListChanged(Tox* tox, uint32_t conference_number, void* user_data);
    static void onFriendLossyPacket(Tox* tox, uint32_t friend_number, const uint8_t* data, size_t length, void* user_data);
    static void onFriendLosslessPacket(Tox* tox, uint32_t friend_number, const uint8_t* data, size_t length, void* user_data);

private:
    friend void onSelfConnectionStatus(Tox* tox, TOX_CONNECTION connection_status, void* user_data);
    friend void onFriendRequest(Tox* tox, const uint8_t* public_key, const uint8_t* message, size_t length, void* user_data);
    friend void onFriendMessage(Tox* tox, uint32_t friend_number, TOX_MESSAGE_TYPE type, const uint8_t* message, size_t length, void* user_data);
    friend void onFriendName(Tox* tox, uint32_t friend_number, const uint8_t* name, size_t length, void* user_data);
    friend void onFriendStatusMessage(Tox* tox, uint32_t friend_number, const uint8_t* message, size_t length, void* user_data);
    friend void onFriendStatus(Tox* tox, uint32_t friend_number, TOX_USER_STATUS status, void* user_data);
    friend void onFriendConnectionStatus(Tox* tox, uint32_t friend_number, TOX_CONNECTION connection_status, void* user_data);
    friend void onFriendReadReceipt(Tox* tox, uint32_t friend_number, uint32_t message_id, void* user_data);
    friend void onFriendTyping(Tox* tox, uint32_t friend_number, bool typing, void* user_data);
    friend void onFileRecv(Tox* tox, uint32_t friend_number, uint32_t file_number, uint32_t kind, uint64_t file_size, const uint8_t* filename, size_t filename_length, void* user_data);
    friend void onFileControl(Tox* tox, uint32_t friend_number, uint32_t file_number, TOX_FILE_CONTROL control, void* user_data);
    friend void onFileChunkRequest(Tox* tox, uint32_t friend_number, uint32_t file_number, uint64_t position, size_t length, void* user_data);
    friend void onFileRecvChunk(Tox* tox, uint32_t friend_number, uint32_t file_number, uint64_t position, const uint8_t* data, size_t length, void* user_data);
    friend void onGroupInvite(Tox* tox, uint32_t friend_number, TOX_CONFERENCE_TYPE type, const uint8_t* cookie, size_t length, void* user_data);
    friend void onGroupMessage(Tox* tox, uint32_t conference_number, uint32_t peer_number, TOX_MESSAGE_TYPE type, const uint8_t* message, size_t length, void* user_data);
    friend void onGroupTitle(Tox* tox, uint32_t conference_number, uint32_t peer_number, const uint8_t* title, size_t length, void* user_data);
    friend void onGroupPeerName(Tox* tox, uint32_t conference_number, uint32_t peer_number, const uint8_t* name, size_t length, void* user_data);
    friend void onGroupPeerListChanged(Tox* tox, uint32_t conference_number, void* user_data);
    friend void onFriendLossyPacket(Tox* tox, uint32_t friend_number, const uint8_t* data, size_t length, void* user_data);
    friend void onFriendLosslessPacket(Tox* tox, uint32_t friend_number, const uint8_t* data, size_t length, void* user_data);

    // 私有构造函数/析构函数
    ToxManager();
    ~ToxManager() = default;

    // 自定义删除器
    static void toxDeleter(Tox* tox);

    // 成员变量
    std::unique_ptr<Tox, decltype(&toxDeleter)> tox_;
    mutable std::mutex mutex_;

    // 回调存储
    SelfConnectionStatusCallback self_connection_status_cb_;
    FriendRequestCallback friend_request_cb_;
    FriendMessageCallback friend_message_cb_;
    FriendNameCallback friend_name_cb_;
    FriendStatusMessageCallback friend_status_message_cb_;
    FriendStatusCallback friend_status_cb_;
    FriendConnectionStatusCallback friend_connection_status_cb_;
    FriendReadReceiptCallback friend_read_receipt_cb_;
    FriendTypingCallback friend_typing_cb_;
    FileRecvCallback file_recv_cb_;
    FileControlCallback file_control_cb_;
    FileChunkRequestCallback file_chunk_request_cb_;
    FileRecvChunkCallback file_recv_chunk_cb_;
    GroupInviteCallback group_invite_cb_;
    GroupMessageCallback group_message_cb_;
    GroupTitleCallback group_title_cb_;
    GroupPeerNameCallback group_peer_name_cb_;
    GroupPeerListChangedCallback group_peer_list_changed_cb_;
    FriendLossyPacketCallback friend_lossy_packet_cb_;
    FriendLosslessPacketCallback friend_lossless_packet_cb_;
    AudioReceiveCallback audio_receive_cb_;
};