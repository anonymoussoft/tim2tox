#ifndef __V2TIM_SIGNALING_MANAGER_IMPL_H__
#define __V2TIM_SIGNALING_MANAGER_IMPL_H__

#include "tox.h"
#include <mutex>
#include <unordered_map>
#include <vector>

#include "V2TIMSignalingManager.h"
#include "V2TIMString.h"
#include "V2TIMUtils.h"

// Forward declarations
struct SignalingPacket;
class V2TIMManagerImpl;
class ToxManager;

struct SignalingInviteInfo {
    V2TIMString inviteID;
    V2TIMString inviter;
    V2TIMString groupID;
    V2TIMString data;
    uint32_t timeout;
    bool isOnlineOnly;
    uint32_t sender_friend_number;
};

class V2TIMSignalingManagerImpl : public V2TIMSignalingManager {
public:
    V2TIMSignalingManagerImpl();
    ~V2TIMSignalingManagerImpl() override;

    // Multi-instance support: Set the associated V2TIMManagerImpl instance
    void SetManagerImpl(V2TIMManagerImpl* manager_impl);

    void AddSignalingListener(V2TIMSignalingListener* listener) override;
    void RemoveSignalingListener(V2TIMSignalingListener* listener) override;
    V2TIMString Invite(const V2TIMString& invitee, const V2TIMString& data, bool onlineUserOnly,
                       const V2TIMOfflinePushInfo& offlinePushInfo, int timeout,
                       V2TIMCallback* callback) override;
    V2TIMString InviteInGroup(const V2TIMString& groupID, const V2TIMStringVector& inviteeList,
                              const V2TIMString& data, bool onlineUserOnly, int timeout,
                              V2TIMCallback* callback) override;
    void Cancel(const V2TIMString& inviteID, const V2TIMString& data,
                V2TIMCallback* callback) override;
    void Accept(const V2TIMString& inviteID, const V2TIMString& data,
                V2TIMCallback* callback) override;
    void Reject(const V2TIMString& inviteID, const V2TIMString& data,
                V2TIMCallback* callback) override;
    V2TIMSignalingInfo GetSignalingInfo(const V2TIMMessage& msg) override;
    void AddInvitedSignaling(const V2TIMSignalingInfo& info, V2TIMCallback* callback) override;
    void ModifyInvitation(const V2TIMString& inviteID, const V2TIMString& data,
                          V2TIMCallback* callback) override;

    void OnToxMessage(uint32_t friend_number, const uint8_t* data, size_t length);
    std::string GetUserIDFromFriendNumber(uint32_t friend_number);
    uint32_t GetFriendNumber(const V2TIMString& userID);

private:
    // Helper methods
    std::string GenerateUniqueID();
    std::vector<uint8_t> SerializePacket(const SignalingPacket& packet);
    void SendToxPacket(uint32_t friend_number, const SignalingPacket& packet);
    uint32_t GetOrCreateConference(const V2TIMString& groupID);
    void SendConferenceMessage(uint32_t conference_number, const V2TIMString& data);
    
    // Helper function to get ToxManager from current V2TIMManagerImpl instance
    ToxManager* GetToxManager();

    // 监听器列表
    std::vector<V2TIMSignalingListener*> listeners_;
    // 邀请信息表 (inviteID -> 信息) — 发送方维护
    std::unordered_map<std::string, SignalingInviteInfo> active_invites_;
    // 接收到的邀请 (inviteID -> inviter_friend_number) — 接收方维护，用于 Accept/Reject
    std::unordered_map<std::string, uint32_t> received_invites_;
    std::mutex mutex_;
    
    // Reference to V2TIMManagerImpl for multi-instance support
    V2TIMManagerImpl* manager_impl_;
    std::mutex manager_impl_mutex_;
    
    // Per-instance counter for unique ID generation (replaces static counter for multi-instance support)
    int unique_id_counter_;
};

#endif  // __V2TIM_SIGNALING_MANAGER_IMPL_H__
