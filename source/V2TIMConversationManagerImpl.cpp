#include "V2TIMConversationManagerImpl.h"
#include "toxcore/tox.h"
#include <algorithm>
#include "ToxManager.h"
#include <cstdio>

// Conversation-specific error codes
namespace {
    constexpr int ERR_SDK_CONVERSATION_NOT_FOUND = 6010;
}

V2TIMConversationManagerImpl::V2TIMConversationManagerImpl() {    
    // Refresh conversation cache
    RefreshConversationCache();
}

V2TIMConversationManagerImpl::~V2TIMConversationManagerImpl() {
    // 清理资源
}

// 监听器管理
void V2TIMConversationManagerImpl::AddConversationListener(V2TIMConversationListener* listener) {
    std::lock_guard<std::mutex> lock(listeners_mutex_);
    listeners_.push_back(listener);
}

void V2TIMConversationManagerImpl::RemoveConversationListener(V2TIMConversationListener* listener) {
    std::lock_guard<std::mutex> lock(listeners_mutex_);
    listeners_.erase(std::remove(listeners_.begin(), listeners_.end(), listener), listeners_.end());
}

// 核心会话列表获取实现
void V2TIMConversationManagerImpl::GetConversationList(uint64_t nextSeq, uint32_t count,
                                                     V2TIMValueCallback<V2TIMConversationResult>* callback) {
    RefreshConversationCache();
    
    V2TIMConversationResult result;
    result.nextSeq = 0;
    result.isFinished = true;
    
    std::lock_guard<std::mutex> lock(cache_mutex_);
    const uint32_t total = cached_conversations_.size();
    const uint32_t start = std::min((uint32_t)nextSeq, total);
    const uint32_t end = std::min(start + count, total);
    
    for (uint32_t i = start; i < end; ++i) {
        result.conversationList.PushBack(cached_conversations_[i]);
    }
    
    if (callback) {
        callback->OnSuccess(result);
    }
}

// 创建会话对象（好友）
V2TIMConversation V2TIMConversationManagerImpl::CreateConversationFromFriend(uint32_t friend_number) {
    V2TIMConversation conv;
    conv.conversationID = "c2c_" + std::to_string(friend_number);
    conv.type = V2TIM_C2C;
    
    uint8_t pubkey[TOX_PUBLIC_KEY_SIZE];
    if (tox_friend_get_public_key(ToxManager::getInstance().getTox(), friend_number, pubkey, nullptr)) {
        // Convert binary pubkey to hex string
        char hex_pubkey[TOX_PUBLIC_KEY_SIZE * 2 + 1];
        for (size_t i = 0; i < TOX_PUBLIC_KEY_SIZE; i++) {
            sprintf(hex_pubkey + i * 2, "%02X", pubkey[i]);
        }
        hex_pubkey[TOX_PUBLIC_KEY_SIZE * 2] = '\0';
        conv.userID = V2TIMString(hex_pubkey);
    }
    
    // 其他字段根据tox状态填充...
    return conv;
}

// 刷新本地缓存
void V2TIMConversationManagerImpl::RefreshConversationCache() {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    cached_conversations_.clear();
    
    // 添加好友会话
    const size_t friend_count = tox_self_get_friend_list_size(ToxManager::getInstance().getTox());
    std::vector<uint32_t> friends(friend_count);
    tox_self_get_friend_list(ToxManager::getInstance().getTox(), friends.data());
    
    for (uint32_t friend_number : friends) {
        cached_conversations_.push_back(CreateConversationFromFriend(friend_number));
    }
    
    // TODO: 添加群组会话（需要根据实际群组实现）
}

// 其他接口实现（部分示例）
void V2TIMConversationManagerImpl::DeleteConversation(const V2TIMString& conversationID, V2TIMCallback* callback) {
    // Check if conversation starts with "c2c_"
    std::string conv_id = conversationID.CString();
    if (conv_id.substr(0, 4) == "c2c_") {
        // Extract friend number after the "c2c_" prefix
        uint32_t friend_number = std::stoul(conv_id.substr(4));
        tox_friend_delete(ToxManager::getInstance().getTox(), friend_number, nullptr);
    }
    
    if (callback) {
        callback->OnSuccess();
    }
}

// 未实现的功能保留空实现
void V2TIMConversationManagerImpl::CreateConversationGroup(const V2TIMString &groupName, 
                                                         const V2TIMStringVector &conversationIDList,
                                                         V2TIMValueCallback<V2TIMConversationOperationResultVector>* callback) {
    // 空实现
    if (callback) {
        V2TIMConversationOperationResultVector result;
        callback->OnSuccess(result);
    }
}

// ...existing code...

void V2TIMConversationManagerImpl::GetConversation(const V2TIMString& conversationID,
                                                   V2TIMValueCallback<V2TIMConversation>* callback) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    auto it = std::find_if(cached_conversations_.begin(), cached_conversations_.end(),
                           [&](const V2TIMConversation& conv) { return conv.conversationID == conversationID; });
    if (it != cached_conversations_.end()) {
        if (callback) callback->OnSuccess(*it);
    } else {
        if (callback) callback->OnError(ERR_SDK_CONVERSATION_NOT_FOUND, "Conversation not found");
    }
}

void V2TIMConversationManagerImpl::GetConversationList(const V2TIMStringVector& conversationIDList,
                                                       V2TIMValueCallback<V2TIMConversationVector>* callback) {
    V2TIMConversationVector result;
    std::lock_guard<std::mutex> lock(cache_mutex_);
    for (const auto& conversationID : conversationIDList) {
        auto it = std::find_if(cached_conversations_.begin(), cached_conversations_.end(),
                               [&](const V2TIMConversation& conv) { return conv.conversationID == conversationID; });
        if (it != cached_conversations_.end()) {
            result.PushBack(*it);
        }
    }
    if (callback) callback->OnSuccess(result);
}

void V2TIMConversationManagerImpl::GetConversationListByFilter(const V2TIMConversationListFilter &filter,
                                                               uint64_t nextSeq, uint32_t count,
                                                               V2TIMValueCallback<V2TIMConversationResult>* callback) {
    // Implementation based on filter criteria
    // For simplicity, returning all conversations
    GetConversationList(nextSeq, count, callback);
}

void V2TIMConversationManagerImpl::DeleteConversationList(const V2TIMStringVector& conversationIDList, bool clearMessage,
                                                          V2TIMValueCallback<V2TIMConversationOperationResultVector>* callback) {
    V2TIMConversationOperationResultVector results;
    for (const auto& conversationID : conversationIDList) {
        DeleteConversation(conversationID, nullptr);
        V2TIMConversationOperationResult result;
        result.conversationID = conversationID;
        result.resultCode = 0;
        results.PushBack(result);
    }
    if (callback) callback->OnSuccess(results);
}

void V2TIMConversationManagerImpl::SetConversationDraft(const V2TIMString& conversationID,
                                                        const V2TIMString& draftText, V2TIMCallback* callback) {
    // Implementation to set conversation draft
    if (callback) callback->OnSuccess();
}

void V2TIMConversationManagerImpl::SetConversationCustomData(const V2TIMStringVector &conversationIDList, const V2TIMBuffer &customData,
                                                             V2TIMValueCallback<V2TIMConversationOperationResultVector>* callback) {
    // Implementation to set custom data
    V2TIMConversationOperationResultVector results;
    for (const auto& conversationID : conversationIDList) {
        V2TIMConversationOperationResult result;
        result.conversationID = conversationID;
        result.resultCode = 0;
        results.PushBack(result);
    }
    if (callback) callback->OnSuccess(results);
}

void V2TIMConversationManagerImpl::PinConversation(const V2TIMString& conversationID, bool isPinned,
                                                   V2TIMCallback* callback) {
    // Implementation to pin conversation
    if (callback) callback->OnSuccess();
}

void V2TIMConversationManagerImpl::MarkConversation(const V2TIMStringVector &conversationIDList, uint64_t markType, bool enableMark,
                                                    V2TIMValueCallback<V2TIMConversationOperationResultVector>* callback) {
    // Implementation to mark conversation
    V2TIMConversationOperationResultVector results;
    for (const auto& conversationID : conversationIDList) {
        V2TIMConversationOperationResult result;
        result.conversationID = conversationID;
        result.resultCode = 0;
        results.PushBack(result);
    }
    if (callback) callback->OnSuccess(results);
}

void V2TIMConversationManagerImpl::GetTotalUnreadMessageCount(V2TIMValueCallback<uint64_t>* callback) {
    // Implementation to get total unread message count
    uint64_t totalUnreadCount = 0;
    if (callback) callback->OnSuccess(totalUnreadCount);
}

void V2TIMConversationManagerImpl::GetUnreadMessageCountByFilter(const V2TIMConversationListFilter &filter,
                                                                 V2TIMValueCallback<uint64_t>* callback) {
    // Implementation to get unread message count by filter
    uint64_t unreadCount = 0;
    if (callback) callback->OnSuccess(unreadCount);
}

void V2TIMConversationManagerImpl::SubscribeUnreadMessageCountByFilter(const V2TIMConversationListFilter &filter) {
    // Implementation to subscribe unread message count by filter
}

void V2TIMConversationManagerImpl::UnsubscribeUnreadMessageCountByFilter(const V2TIMConversationListFilter &filter) {
    // Implementation to unsubscribe unread message count by filter
}

void V2TIMConversationManagerImpl::CleanConversationUnreadMessageCount(const V2TIMString& conversationID, uint64_t cleanTimestamp, uint64_t cleanSequence,
                                                                       V2TIMCallback* callback) {
    // Implementation to clean conversation unread message count
    if (callback) callback->OnSuccess();
}

void V2TIMConversationManagerImpl::GetConversationGroupList(V2TIMValueCallback<V2TIMStringVector>* callback) {
    // Empty implementation
    V2TIMStringVector result;
    if (callback) callback->OnSuccess(result);
}

void V2TIMConversationManagerImpl::DeleteConversationGroup(const V2TIMString &groupName, V2TIMCallback* callback) {
    // Empty implementation
    if (callback) callback->OnSuccess();
}

void V2TIMConversationManagerImpl::RenameConversationGroup(const V2TIMString &oldName, const V2TIMString &newName,
                                                           V2TIMCallback* callback) {
    // Empty implementation
    if (callback) callback->OnSuccess();
}

void V2TIMConversationManagerImpl::AddConversationsToGroup(const V2TIMString &groupName, const V2TIMStringVector &conversationIDList,
                                                           V2TIMValueCallback<V2TIMConversationOperationResultVector>* callback) {
    // Empty implementation
    V2TIMConversationOperationResultVector result;
    if (callback) callback->OnSuccess(result);
}

void V2TIMConversationManagerImpl::DeleteConversationsFromGroup(const V2TIMString &groupName, const V2TIMStringVector &conversationIDList,
                                                                V2TIMValueCallback<V2TIMConversationOperationResultVector>* callback) {
    // Empty implementation
    V2TIMConversationOperationResultVector result;
    if (callback) callback->OnSuccess(result);
}