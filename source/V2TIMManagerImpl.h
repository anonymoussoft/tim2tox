#ifndef __V2TIM_MANAGER_IMPL_H__
#define __V2TIM_MANAGER_IMPL_H__

#include <mutex>
#include "toxcore/tox_struct.h"
#include "V2TIMManager.h"
#include "V2TIMMessageManager.h"
#include "V2TIMGroupManager.h"
#include "V2TIMConversationManager.h"
#include "V2TIMFriendshipManager.h"
#include "V2TIMOfflinePushManager.h"
#include "V2TIMSignalingManager.h"
#include "V2TIMCommunityManager.h"
#include "ToxManager.h"
#include <memory>
#include <vector>
#include <unordered_set>
#include <thread>
#include "V2TIMStringHash.h"
#include <unordered_map>

class V2TIMManagerImpl : public V2TIMManager {
public:
    static V2TIMManagerImpl* GetInstance();

    // SDK Listener Management
    void AddSDKListener(V2TIMSDKListener* listener) override;
    void RemoveSDKListener(V2TIMSDKListener* listener) override;

    // SDK Initialization and Cleanup
    bool InitSDK(uint32_t sdkAppID, const V2TIMSDKConfig& config) override;
    void UnInitSDK() override;

    // SDK Information
    V2TIMString GetVersion() override;
    int64_t GetServerTime() override;

    // User Authentication
    void Login(const V2TIMString& userID, const V2TIMString& userSig, V2TIMCallback* callback) override;
    void Logout(V2TIMCallback* callback) override;
    V2TIMString GetLoginUser() override;
    V2TIMLoginStatus GetLoginStatus() override;

    // Messaging
    void AddSimpleMsgListener(V2TIMSimpleMsgListener* listener) override;
    void RemoveSimpleMsgListener(V2TIMSimpleMsgListener* listener) override;
    V2TIMString SendC2CTextMessage(const V2TIMString& text, const V2TIMString& userID, V2TIMSendCallback* callback) override;
    V2TIMString SendC2CCustomMessage(const V2TIMBuffer& customData, const V2TIMString& userID, V2TIMSendCallback* callback) override;
    V2TIMString SendGroupTextMessage(const V2TIMString& text, const V2TIMString& groupID, V2TIMMessagePriority priority, V2TIMSendCallback* callback) override;
    V2TIMString SendGroupCustomMessage(const V2TIMBuffer& customData, const V2TIMString& groupID, V2TIMMessagePriority priority, V2TIMSendCallback* callback) override;

    // Group Management
    void AddGroupListener(V2TIMGroupListener* listener) override;
    void RemoveGroupListener(V2TIMGroupListener* listener) override;
    void CreateGroup(const V2TIMString& groupType, const V2TIMString& groupID, const V2TIMString& groupName, V2TIMValueCallback<V2TIMString>* callback) override;
    void JoinGroup(const V2TIMString& groupID, const V2TIMString& message, V2TIMCallback* callback) override;
    void QuitGroup(const V2TIMString& groupID, V2TIMCallback* callback) override;
    void DismissGroup(const V2TIMString& groupID, V2TIMCallback* callback) override;

    // User Info
    void GetUsersInfo(const V2TIMStringVector& userIDList, V2TIMValueCallback<V2TIMUserFullInfoVector>* callback) override;
    void SetSelfInfo(const V2TIMUserFullInfo& info, V2TIMCallback* callback) override;

    // Search & Status
    void SearchUsers(const V2TIMUserSearchParam& param, V2TIMValueCallback<V2TIMUserSearchResult>* callback) override;
    void GetUserStatus(const V2TIMStringVector& userIDList, V2TIMValueCallback<V2TIMUserStatusVector>* callback) override;
    void SetSelfStatus(const V2TIMUserStatus& status, V2TIMCallback* callback) override;
    void SubscribeUserStatus(const V2TIMStringVector& userIDList, V2TIMCallback* callback) override;
    void UnsubscribeUserStatus(const V2TIMStringVector& userIDList, V2TIMCallback* callback) override;
    void SubscribeUserInfo(const V2TIMStringVector& userIDList, V2TIMCallback* callback) override;
    void UnsubscribeUserInfo(const V2TIMStringVector& userIDList, V2TIMCallback* callback) override;

    // Advanced Managers
    V2TIMMessageManager* GetMessageManager() override;
    V2TIMGroupManager* GetGroupManager() override;
    V2TIMCommunityManager* GetCommunityManager() override;
    V2TIMConversationManager* GetConversationManager() override;
    V2TIMFriendshipManager* GetFriendshipManager() override;
    V2TIMOfflinePushManager* GetOfflinePushManager() override;
    V2TIMSignalingManager* GetSignalingManager() override;

    // Experimental API
    void CallExperimentalAPI(const V2TIMString& api, const void* param, V2TIMValueCallback<V2TIMBaseObject>* callback) override;

    // Helper methods for accessing private data
    bool GetGroupNumberFromID(const V2TIMString& groupID, uint32_t& group_number);

private:
    // Remove Tox instance and use ToxManager singleton
    std::thread event_thread_;
    bool running_ = true;
    V2TIMString logged_in_user_;
    std::mutex mutex_;

    // --- Listener Sets ---
    std::unordered_set<V2TIMSDKListener*> sdk_listeners_;
    std::unordered_set<V2TIMSimpleMsgListener*> simple_msg_listeners_;
    std::unordered_set<V2TIMGroupListener*> group_listeners_;

    // --- Mappings ---
    // Map V2TIM GroupID string to Tox conference_number
    std::unordered_map<V2TIMString, uint32_t> group_id_to_conference_number_;
    // Map Tox conference_number back to V2TIM GroupID string (for receiving messages)
    std::unordered_map<uint32_t, V2TIMString> conference_number_to_group_id_;
    
    // TODO: Consider mapping for friend numbers to UserIDs if needed frequently
    // std::unordered_map<uint32_t, V2TIMString> friend_number_to_user_id_;
    // std::unordered_map<V2TIMString, uint32_t> user_id_to_friend_number_;

    // --- Internal Handler Methods ---
    void HandleGroupMessage(uint32_t conference_number, uint32_t peer_number, TOX_MESSAGE_TYPE type, const uint8_t* message, size_t length);
    void HandleFriendMessage(uint32_t friend_number, TOX_MESSAGE_TYPE type, const uint8_t* message, size_t length);
    void HandleSelfConnectionStatus(TOX_CONNECTION connection_status);
    void HandleFriendRequest(const uint8_t* public_key, const uint8_t* message, size_t length);
    void HandleFriendName(uint32_t friend_number, const uint8_t* name, size_t length);
    void HandleFriendStatusMessage(uint32_t friend_number, const uint8_t* message, size_t length);
    void HandleFriendStatus(uint32_t friend_number, TOX_USER_STATUS status);
    void HandleGroupTitle(uint32_t conference_number, uint32_t peer_number, const uint8_t* title, size_t length);
    void HandleGroupPeerName(uint32_t conference_number, uint32_t peer_number, const uint8_t* name, size_t length);
    void HandleGroupPeerListChanged(uint32_t conference_number);
    // TODO: Add handlers for other Tox callbacks (read receipts, file transfer, etc.)

    // Private constructor/destructor if using singleton pattern correctly
    V2TIMManagerImpl(); 
    ~V2TIMManagerImpl();

    // Delete copy constructor and assignment operator
    V2TIMManagerImpl(const V2TIMManagerImpl&) = delete;
    V2TIMManagerImpl& operator=(const V2TIMManagerImpl&) = delete;

    friend class V2TIMManager; // Allow V2TIMManager::GetInstance() potentially
};

#endif // __V2TIM_MANAGER_IMPL_H__
