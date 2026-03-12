#ifndef __V2TIM_COMMUNITY_MANAGER_IMPL_H__
#define __V2TIM_COMMUNITY_MANAGER_IMPL_H__

#include "V2TIMCommunityManager.h"
#include "tox.h"
#include <mutex>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <sqlite3.h>
#include <string>

// Forward declaration
class V2TIMManagerImpl;
class ToxManager;
class V2TIMCommunityManagerImpl;

// Forward declaration for namespace function
namespace community {
    ToxManager* GetToxManager(V2TIMCommunityManagerImpl* self);
}

// Error codes
static constexpr int ERR_COMM_API_CALL_FAILED = -100;
static constexpr int ERR_SDK_COMMUNITY_NOT_FOUND = -103;

class V2TIMCommunityManagerImpl : public V2TIMCommunityManager {
public:
    static V2TIMCommunityManagerImpl& getInstance();
    
    // Multi-instance support: Set the associated V2TIMManagerImpl instance
    void SetManagerImpl(V2TIMManagerImpl* manager_impl);
    
    // Friend function to access private members
    friend ToxManager* community::GetToxManager(V2TIMCommunityManagerImpl* self);
    
    // Community listener management
    void AddCommunityListener(V2TIMCommunityListener* listener) override;
    void RemoveCommunityListener(V2TIMCommunityListener* listener) override;
    
    // Basic community management
    void CreateCommunity(const V2TIMGroupInfo& info, 
                        const V2TIMCreateGroupMemberInfoVector& memberList,
                        V2TIMValueCallback<V2TIMString>* callback) override;
    void GetJoinedCommunityList(V2TIMValueCallback<V2TIMGroupInfoVector>* callback) override;
    
    // Topic management
    void CreateTopicInCommunity(const V2TIMString& groupID, 
                               const V2TIMTopicInfo& topicInfo,
                               V2TIMValueCallback<V2TIMString>* callback) override;
    void DeleteTopicFromCommunity(const V2TIMString& groupID, 
                                 const V2TIMStringVector& topicIDList,
                                 V2TIMValueCallback<V2TIMTopicOperationResultVector>* callback) override;
    void SetTopicInfo(const V2TIMTopicInfo& topicInfo, V2TIMCallback* callback) override;
    void GetTopicInfoList(const V2TIMString& groupID, 
                         const V2TIMStringVector& topicIDList,
                         V2TIMValueCallback<V2TIMTopicInfoResultVector>* callback) override;
    
    // Additional methods                     
    void GetCommunityMemberList(const V2TIMString& communityID,
                              uint32_t filter,
                              uint32_t nextSeq,
                              V2TIMValueCallback<V2TIMGroupMemberResult>* callback);

    // Permission group management
    void CreatePermissionGroupInCommunity(const V2TIMPermissionGroupInfo& info,
                                        V2TIMValueCallback<V2TIMString>* callback) override;
    void DeletePermissionGroupFromCommunity(const V2TIMString& groupID,
                                          const V2TIMStringVector& permissionGroupIDList,
                                          V2TIMValueCallback<V2TIMPermissionGroupOperationResultVector>* callback) override;
    void ModifyPermissionGroupInfoInCommunity(const V2TIMPermissionGroupInfo& info,
                                            V2TIMCallback* callback) override;
    void GetJoinedPermissionGroupListInCommunity(const V2TIMString& groupID,
                                                V2TIMValueCallback<V2TIMPermissionGroupInfoResultVector>* callback) override;
    void GetPermissionGroupListInCommunity(const V2TIMString& groupID,
                                         const V2TIMStringVector& permissionGroupIDList,
                                         V2TIMValueCallback<V2TIMPermissionGroupInfoResultVector>* callback) override;
    void AddCommunityMembersToPermissionGroup(const V2TIMString& groupID,
                                            const V2TIMString& permissionGroupID,
                                            const V2TIMStringVector& memberList,
                                            V2TIMValueCallback<V2TIMPermissionGroupMemberOperationResultVector>* callback) override;
    void RemoveCommunityMembersFromPermissionGroup(const V2TIMString& groupID,
                                                  const V2TIMString& permissionGroupID,
                                                  const V2TIMStringVector& memberList,
                                                  V2TIMValueCallback<V2TIMPermissionGroupMemberOperationResultVector>* callback) override;
    void GetCommunityMemberListInPermissionGroup(const V2TIMString& groupID,
                                               const V2TIMString& permissionGroupID,
                                               const V2TIMString& nextCursor,
                                               V2TIMValueCallback<V2TIMPermissionGroupMemberInfoResult>* callback) override;
    void AddTopicPermissionToPermissionGroup(const V2TIMString& groupID,
                                           const V2TIMString& permissionGroupID,
                                           const V2TIMStringToUint64Map& topicPermissionMap,
                                           V2TIMValueCallback<V2TIMTopicOperationResultVector>* callback) override;
    void DeleteTopicPermissionFromPermissionGroup(const V2TIMString& groupID,
                                                const V2TIMString& permissionGroupID,
                                                const V2TIMStringVector& topicIDList,
                                                V2TIMValueCallback<V2TIMTopicOperationResultVector>* callback) override;
    void ModifyTopicPermissionInPermissionGroup(const V2TIMString& groupID,
                                              const V2TIMString& permissionGroupID,
                                              const V2TIMStringToUint64Map& topicPermissionMap,
                                              V2TIMValueCallback<V2TIMTopicOperationResultVector>* callback) override;
    void GetTopicPermissionInPermissionGroup(const V2TIMString& groupID,
                                           const V2TIMString& permissionGroupID,
                                           const V2TIMStringVector& topicIDList,
                                           V2TIMValueCallback<V2TIMTopicPermissionResultVector>* callback) override;

private:
    V2TIMCommunityManagerImpl();
    ~V2TIMCommunityManagerImpl() = default;
    
    // Prevent copying
    V2TIMCommunityManagerImpl(const V2TIMCommunityManagerImpl&) = delete;
    V2TIMCommunityManagerImpl& operator=(const V2TIMCommunityManagerImpl&) = delete;
    
    // Helper methods
    V2TIMCallback* InviteMemberToCommunity(const std::string& communityID, const std::string& userID);
    uint32_t GetFriendNumber(const std::string& userID);
    
    // Member variables
    std::mutex mutex_;
    std::mutex community_mutex_;
    std::mutex topic_mutex_;
    std::mutex listener_mutex_;
    
    std::unordered_set<V2TIMCommunityListener*> listeners_;
    std::unordered_map<std::string, uint32_t> communities_;  // communityID -> conference_number
    std::unordered_map<std::string, V2TIMGroupInfo> community_info_;
    std::unordered_map<std::string, std::vector<std::pair<std::string, V2TIMTopicInfo>>> topics_;
    std::unordered_map<std::string, uint32_t> friend_numbers_;
    
    sqlite3* db_;
    uint32_t topic_counter_ = 0;
    
    // Reference to V2TIMManagerImpl for multi-instance support
    V2TIMManagerImpl* manager_impl_;
    std::mutex manager_impl_mutex_;
};

#endif // __V2TIM_COMMUNITY_MANAGER_IMPL_H__
