#ifndef V2TIM_GROUP_MANAGER_IMPL_H
#define V2TIM_GROUP_MANAGER_IMPL_H

#include "V2TIMGroupManager.h"
#include "V2TIMManagerImpl.h" // For access to core manager functionality if needed
#include "V2TIMString.h"
#include "V2TIMValueCallback.h"
#include "V2TIMCallback.h"
#include "V2TIMDefine.h" // For V2TIM_CALLBACK, etc.

namespace v2im {

class V2TIMGroupManagerImpl : public V2TIMGroupManager {
public:
    explicit V2TIMGroupManagerImpl(V2TIMManagerImpl* manager_impl);
    ~V2TIMGroupManagerImpl() override = default;

    // V2TIMGroupManager interface
    void CreateGroup(const V2TIMString& groupType, const V2TIMString& groupID, const V2TIMString& groupName,
                     V2TIMValueCallback<V2TIMString>* callback) override;

    void JoinGroup(const V2TIMString& groupID, const V2TIMString& message,
                   V2TIMCallback* callback) override;

    void QuitGroup(const V2TIMString& groupID, V2TIMCallback* callback) override;

    void DismissGroup(const V2TIMString& groupID, V2TIMCallback* callback) override;

    void GetJoinedGroupList(V2TIMValueCallback<V2TIMGroupInfoVector>* callback) override;

    void GetGroupInfo(const V2TIMStringVector& groupIDList,
                      V2TIMValueCallback<V2TIMGroupInfoResultVector>* callback) override;

    void SetGroupInfo(const V2TIMGroupInfo& info, V2TIMCallback* callback) override;

    void GetGroupMemberList(const V2TIMString& groupID, uint32_t filter, uint64_t nextSeq,
                           V2TIMValueCallback<V2TIMGroupMemberInfoResult>* callback) override;

    void GetGroupMembersInfo(const V2TIMString& groupID, const V2TIMStringVector& memberList,
                            V2TIMValueCallback<V2TIMGroupMemberInfoResultVector>* callback) override;

    void SetGroupMemberInfo(const V2TIMString& groupID, const V2TIMGroupMemberInfo& info,
                            V2TIMCallback* callback) override;

    void MuteGroupMember(const V2TIMString& groupID, const V2TIMString& userID, uint32_t seconds,
                         V2TIMCallback* callback) override;

    void KickGroupMember(const V2TIMString& groupID, const V2TIMStringVector& memberList,
                         const V2TIMString& reason, V2TIMValueCallback<V2TIMGroupMemberOperationResultVector>* callback) override;

    void SetGroupMemberRole(const V2TIMString& groupID, const V2TIMString& userID, uint32_t role,
                            V2TIMCallback* callback) override;

    void TransferGroupOwner(const V2TIMString& groupID, const V2TIMString& userID,
                            V2TIMCallback* callback) override;

    void InviteUserToGroup(const V2TIMString& groupID, const V2TIMStringVector& userList,
                          V2TIMValueCallback<V2TIMGroupMemberOperationResultVector>* callback) override;

    // --- Group Attributes (Potentially map to Tox group metadata/status messages?) ---
    // void InitGroupAttributes(...) override;
    // void SetGroupAttributes(...) override;
    // void DeleteGroupAttributes(...) override;
    // void GetGroupAttributes(...) override;

    // --- Group Online Member Count (Likely requires manual tracking) ---
    // void GetGroupOnlineMemberCount(...) override;

    // --- Topic related methods (Not directly applicable to Tox groups) ---
    // ... skip topic methods ...

    // --- Community related methods (Not directly applicable to Tox groups) ---
    // ... skip community methods ...

    // --- Search methods ---
    // void SearchGroups(...) override;
    // void SearchGroupMembers(...) override;

    // --- MarkAsUnread (Not directly applicable to Tox groups) ---
    // void MarkGroupMessageListAsRead(...) override;

private:
    V2TIMManagerImpl* manager_impl_; // Pointer to the core manager
};

} // namespace v2im

#endif // V2TIM_GROUP_MANAGER_IMPL_H 