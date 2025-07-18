#include "V2TIMCommunityManagerImpl.h"
#include "V2TIMErrorCode.h"
#include "ToxManager.h"
#include "V2TIMCallback.h"
#include "V2TIMLog.h"
#include "V2TIMUtils.h"
#include "V2TIMDefine.h"
#include "toxcore/tox.h"
#include <mutex>

// Helper macro for logging with format strings
// These allow us to use string formatting in logs like Python's f-strings
#define V2TIM_LOG_DEBUG(...) V2TIM_LOG(kDebug, __VA_ARGS__)
#define V2TIM_LOG_INFO(...) V2TIM_LOG(kInfo, __VA_ARGS__)
#define V2TIM_LOG_WARNING(...) V2TIM_LOG(kWarning, __VA_ARGS__)
#define V2TIM_LOG_ERROR(...) V2TIM_LOG(kError, __VA_ARGS__)
#define V2TIM_LOG_FATAL(...) V2TIM_LOG(kFatal, __VA_ARGS__)

namespace community {
    // Error codes - renamed to avoid conflicts
    constexpr int ERR_COMM_API_CALL_FAILED = -100;
    constexpr int ERR_COMM_NOT_INITIALIZED = -101;
    constexpr int ERR_COMM_INVALID_PARAMS = -102;
    constexpr int ERR_COMM_NOT_FOUND = -103;
    constexpr int ERR_COMM_SUCCESS = 0;

    // Helper function for hex string conversion
    bool hex_string_to_bin(const char* hex_string, uint8_t* bytes, size_t size) {
        if (!hex_string || !bytes) return false;
        size_t len = strlen(hex_string);
        if (len/2 != size) return false;
        
        for (size_t i = 0; i < size; i++) {
            char hex[3] = {hex_string[i*2], hex_string[i*2+1], '\0'};
            char* end;
            bytes[i] = (uint8_t)strtol(hex, &end, 16);
            if (*end != '\0') return false;
        }
        return true;
    }
}  // namespace community

class InviteMemberCallback : public V2TIMCallback {
public:
    void OnSuccess() override {}
    void OnError(int error_code, const V2TIMString& error_message) override {}
    int code;
    V2TIMString desc;
};

V2TIMCommunityManagerImpl& V2TIMCommunityManagerImpl::getInstance() {
    static V2TIMCommunityManagerImpl instance;
    return instance;
}

// 社群监听器管理
void V2TIMCommunityManagerImpl::AddCommunityListener(V2TIMCommunityListener* listener) {
    std::lock_guard<std::mutex> lock(listener_mutex_);
    listeners_.insert(listener);
}

void V2TIMCommunityManagerImpl::RemoveCommunityListener(V2TIMCommunityListener* listener) {
    std::lock_guard<std::mutex> lock(listener_mutex_);
    listeners_.erase(listener);
}

// 基础社群管理
void V2TIMCommunityManagerImpl::CreateCommunity(const V2TIMGroupInfo& info, 
                                                const V2TIMCreateGroupMemberInfoVector& memberList,
                                                V2TIMValueCallback<V2TIMString>* callback) {
    TOX_ERR_CONFERENCE_NEW err;
    uint32_t conference_number = tox_conference_new(ToxManager::getInstance().getTox(), &err);
    if (err != TOX_ERR_CONFERENCE_NEW_OK) {
        if (callback) callback->OnError(ERR_SDK_INTERNAL_ERROR, "Create failed");
        return;
    }

    std::string communityID = "tox_community_" + std::to_string(conference_number);
    {
        std::lock_guard<std::mutex> lock(community_mutex_);
        communities_[communityID] = conference_number;
        community_info_[communityID] = info;
    }

    // 邀请初始成员
    for (size_t i = 0; i < memberList.Size(); ++i) {
        InviteMemberToCommunity(communityID, std::string(memberList[i].userID.CString()));
    }

    if (callback) callback->OnSuccess(V2TIMString(communityID.c_str()));
}

void V2TIMCommunityManagerImpl::GetJoinedCommunityList(V2TIMValueCallback<V2TIMGroupInfoVector>* callback) {
    V2TIMGroupInfoVector communities;
    std::lock_guard<std::mutex> lock(community_mutex_);
    for (const auto& [id, info] : community_info_) {
        communities.PushBack(info);
    }
    if (callback) callback->OnSuccess(communities);
}

// 话题管理（本地模拟）
void V2TIMCommunityManagerImpl::CreateTopicInCommunity(const V2TIMString& groupID, const V2TIMTopicInfo& topicInfo,
                                                       V2TIMValueCallback<V2TIMString>* callback) {
    std::lock_guard<std::mutex> lock(topic_mutex_);
    std::string topicID = "topic_" + std::to_string(++topic_counter_);
    topics_[std::string(groupID.CString())].push_back({topicID, topicInfo});
    if (callback) callback->OnSuccess(V2TIMString(topicID.c_str()));
}

void V2TIMCommunityManagerImpl::GetTopicInfoList(const V2TIMString& groupID, const V2TIMStringVector& topicIDList,
                                                 V2TIMValueCallback<V2TIMTopicInfoResultVector>* callback) {
    V2TIMTopicInfoResultVector results;
    std::lock_guard<std::mutex> lock(topic_mutex_);
    auto it = topics_.find(std::string(groupID.CString()));
    if (it != topics_.end()) {
        for (const auto& [id, info] : it->second) {
            V2TIMTopicInfoResult result;
            result.topicInfo = info;
            result.errorCode = 0;
            results.PushBack(result);
        }
    }
    if (callback) callback->OnSuccess(results);
}

void V2TIMCommunityManagerImpl::DeleteTopicFromCommunity(const V2TIMString &groupID, const V2TIMStringVector &topicIDList,
                                                        V2TIMValueCallback<V2TIMTopicOperationResultVector> *callback) {
    // TODO: Implement this function
    if (callback) {
        callback->OnSuccess(V2TIMTopicOperationResultVector());
    }
}

void V2TIMCommunityManagerImpl::SetTopicInfo(const V2TIMTopicInfo &topicInfo, V2TIMCallback *callback) {
    // TODO: Implement this function
    if (callback) {
        callback->OnSuccess();
    }
}

// 权限组管理（未实现）
void V2TIMCommunityManagerImpl::CreatePermissionGroupInCommunity(const V2TIMPermissionGroupInfo& info,
                                                                 V2TIMValueCallback<V2TIMString>* callback) {
    if (callback) callback->OnError(ERR_SDK_INTERNAL_ERROR, "Not supported");
}

void V2TIMCommunityManagerImpl::DeletePermissionGroupFromCommunity(const V2TIMString& groupID,
                                                                   const V2TIMStringVector& permissionGroupIDList,
                                                                   V2TIMValueCallback<V2TIMPermissionGroupOperationResultVector>* callback) {
    if (callback) callback->OnError(ERR_SDK_INTERNAL_ERROR, "Not supported");
}

void V2TIMCommunityManagerImpl::ModifyPermissionGroupInfoInCommunity(const V2TIMPermissionGroupInfo &info,
                                                                    V2TIMCallback *callback) {
    // TODO: Implement this function
    if (callback) {
        callback->OnSuccess();
    }
}

void V2TIMCommunityManagerImpl::GetJoinedPermissionGroupListInCommunity(const V2TIMString &groupID,
                                                                       V2TIMValueCallback<V2TIMPermissionGroupInfoResultVector> *callback) {
    // TODO: Implement this function
    if (callback) {
        callback->OnSuccess(V2TIMPermissionGroupInfoResultVector());
    }
}

void V2TIMCommunityManagerImpl::GetPermissionGroupListInCommunity(const V2TIMString &groupID,
                                                                 const V2TIMStringVector &permissionGroupIDList,
                                                                 V2TIMValueCallback<V2TIMPermissionGroupInfoResultVector> *callback) {
    // TODO: Implement this function
    if (callback) {
        callback->OnSuccess(V2TIMPermissionGroupInfoResultVector());
    }
}

void V2TIMCommunityManagerImpl::AddCommunityMembersToPermissionGroup(const V2TIMString &groupID,
                                                                    const V2TIMString &permissionGroupID,
                                                                    const V2TIMStringVector &memberList,
                                                                    V2TIMValueCallback<V2TIMPermissionGroupMemberOperationResultVector> *callback) {
    // TODO: Implement this function
    if (callback) {
        callback->OnSuccess(V2TIMPermissionGroupMemberOperationResultVector());
    }
}

void V2TIMCommunityManagerImpl::RemoveCommunityMembersFromPermissionGroup(const V2TIMString &groupID,
                                                                         const V2TIMString &permissionGroupID,
                                                                         const V2TIMStringVector &memberList,
                                                                         V2TIMValueCallback<V2TIMPermissionGroupMemberOperationResultVector> *callback) {
    // TODO: Implement this function
    if (callback) {
        callback->OnSuccess(V2TIMPermissionGroupMemberOperationResultVector());
    }
}

void V2TIMCommunityManagerImpl::GetCommunityMemberListInPermissionGroup(const V2TIMString &groupID,
                                                                       const V2TIMString &permissionGroupID,
                                                                       const V2TIMString &nextCursor,
                                                                       V2TIMValueCallback<V2TIMPermissionGroupMemberInfoResult> *callback) {
    // TODO: Implement this function
    if (callback) {
        callback->OnSuccess(V2TIMPermissionGroupMemberInfoResult());
    }
}

void V2TIMCommunityManagerImpl::AddTopicPermissionToPermissionGroup(const V2TIMString &groupID,
                                                                   const V2TIMString &permissionGroupID,
                                                                   const V2TIMStringToUint64Map &topicPermissionMap,
                                                                   V2TIMValueCallback<V2TIMTopicOperationResultVector> *callback) {
    // TODO: Implement this function
    if (callback) {
        callback->OnSuccess(V2TIMTopicOperationResultVector());
    }
}

void V2TIMCommunityManagerImpl::DeleteTopicPermissionFromPermissionGroup(const V2TIMString &groupID,
                                                                        const V2TIMString &permissionGroupID,
                                                                        const V2TIMStringVector &topicIDList,
                                                                        V2TIMValueCallback<V2TIMTopicOperationResultVector> *callback) {
    // TODO: Implement this function
    if (callback) {
        callback->OnSuccess(V2TIMTopicOperationResultVector());
    }
}

void V2TIMCommunityManagerImpl::ModifyTopicPermissionInPermissionGroup(const V2TIMString &groupID,
                                                                      const V2TIMString &permissionGroupID,
                                                                      const V2TIMStringToUint64Map &topicPermissionMap,
                                                                      V2TIMValueCallback<V2TIMTopicOperationResultVector> *callback) {
    // TODO: Implement this function
    if (callback) {
        callback->OnSuccess(V2TIMTopicOperationResultVector());
    }
}

void V2TIMCommunityManagerImpl::GetTopicPermissionInPermissionGroup(const V2TIMString &groupID,
                                                                   const V2TIMString &permissionGroupID,
                                                                   const V2TIMStringVector &topicIDList,
                                                                   V2TIMValueCallback<V2TIMTopicPermissionResultVector> *callback) {
    // TODO: Implement this function
    if (callback) {
        callback->OnSuccess(V2TIMTopicPermissionResultVector());
    }
}

// 初始化数据库
V2TIMCommunityManagerImpl::V2TIMCommunityManagerImpl() {    
    sqlite3_open("community_data.db", &db_);
    sqlite3_exec(db_, 
        "CREATE TABLE IF NOT EXISTS communities ("
        "id TEXT PRIMARY KEY, name TEXT, topic_count INTEGER)", 
        nullptr, nullptr, nullptr);
}

// 邀请成员到社群（使用 Tox 会议邀请）
V2TIMCallback* V2TIMCommunityManagerImpl::InviteMemberToCommunity(const std::string& communityID, const std::string& userID) {
    auto* callback = new InviteMemberCallback();
    
    // Find the conference number for this community
    auto it = communities_.find(communityID);
    if (it == communities_.end()) {
        V2TIM_LOG_ERROR("Community not found: {}", communityID);
        callback->code = community::ERR_COMM_INVALID_PARAMS;
        callback->desc = "Community not found";
        return callback;
    }
    
    // Get friend number
    uint32_t friend_number = GetFriendNumber(userID);
    if (friend_number == UINT32_MAX) {
        V2TIM_LOG_ERROR("Failed to get friend number for user: {}", userID);
        callback->code = community::ERR_COMM_INVALID_PARAMS;
        callback->desc = "Friend not found";
        return callback;
    }
    
    // Send conference invite
    auto* tox = ToxManager::getInstance().getTox();
    if (!tox) {
        V2TIM_LOG_ERROR("Failed to get Tox instance");
        callback->code = community::ERR_COMM_NOT_INITIALIZED;
        callback->desc = "Failed to get Tox instance";
        return callback;
    }
    
    TOX_ERR_CONFERENCE_INVITE error;
    tox_conference_invite(tox, friend_number, it->second, &error);
    if (error != TOX_ERR_CONFERENCE_INVITE_OK) {
        V2TIM_LOG_ERROR("Failed to invite {} to conference {}, error: {}", userID, communityID, error);
        callback->code = community::ERR_COMM_API_CALL_FAILED;
        callback->desc = "Failed to send conference invite";
        return callback;
    }
    
    V2TIM_LOG_INFO("Successfully invited {} to community {}", userID, communityID);
    callback->code = community::ERR_COMM_SUCCESS;
    return callback;
}

// 工具函数：userID 到 Tox friend_number 的映射
uint32_t V2TIMCommunityManagerImpl::GetFriendNumber(const std::string& userID) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Check if we already have the friend number cached
    auto it = friend_numbers_.find(userID);
    if (it != friend_numbers_.end()) {
        return it->second;
    }
    
    // Try to find friend by public key
    auto tox = ToxManager::getInstance().getTox();
    if (!tox) {
        V2TIM_LOG_ERROR("Failed to get Tox instance");
        return UINT32_MAX;
    }
    
    // Convert userID to public key and find friend
    uint8_t public_key[TOX_PUBLIC_KEY_SIZE];
    if (!community::hex_string_to_bin(userID.c_str(), public_key, sizeof(public_key))) {
        V2TIM_LOG_ERROR("Invalid public key format: {}", userID);
        return UINT32_MAX;
    }
    
    TOX_ERR_FRIEND_BY_PUBLIC_KEY error;
    uint32_t friend_number = tox_friend_by_public_key(tox, public_key, &error);
    if (error != TOX_ERR_FRIEND_BY_PUBLIC_KEY_OK) {
        V2TIM_LOG_ERROR("Failed to get friend number for {}, error: {}", userID, error);
        return UINT32_MAX;
    }
    
    // Cache the result
    friend_numbers_[userID] = friend_number;
    return friend_number;
}

void V2TIMCommunityManagerImpl::GetCommunityMemberList(const V2TIMString& communityID,
                                                     uint32_t filter,
                                                     uint32_t nextSeq,
                                                     V2TIMValueCallback<V2TIMGroupMemberResult>* callback) {
    if (!callback) return;
    
    auto it = communities_.find(communityID.CString());
    if (it == communities_.end()) {
        V2TIM_LOG_ERROR("Community not found: {}", communityID.CString());
        callback->OnError(community::ERR_COMM_NOT_FOUND, "Community not found");
        return;
    }
    
    // ... rest of the implementation ...
}
