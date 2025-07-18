#include "V2TIMFriendshipManagerImpl.h"
#include "V2TIMUtils.h"
#include "ToxManager.h"
#include "V2TIMErrorCode.h"
#include "V2TIMLog.h"
#include <V2TIMManagerImpl.h> // May need for user info lookups?
#include <vector>
#include <algorithm> // For std::remove
#include "ToxUtils.h" // 添加工具函数头文件

// Helper function to convert hex string to bytes
bool tox_address_string_to_bytes(const char* address_string, uint8_t* bytes) {
    if (!address_string || !bytes) return false;
    size_t len = strlen(address_string);
    if (len != TOX_ADDRESS_SIZE * 2) return false;
    
    for (size_t i = 0; i < TOX_ADDRESS_SIZE; i++) {
        char hex[3] = {address_string[i*2], address_string[i*2+1], '\0'};
        char* end;
        bytes[i] = (uint8_t)strtol(hex, &end, 16);
        if (*end != '\0') return false;
    }
    return true;
}

// Helper function to convert bytes to hex string
static void tox_address_to_string_impl(const uint8_t* bytes, char* address_string) {
    for (size_t i = 0; i < TOX_ADDRESS_SIZE; i++) {
        sprintf(address_string + (i * 2), "%02X", bytes[i]);
    }
    address_string[TOX_ADDRESS_SIZE * 2] = '\0';
}

// Singleton instance
V2TIMFriendshipManagerImpl* V2TIMFriendshipManagerImpl::GetInstance() {
    static V2TIMFriendshipManagerImpl instance;
    return &instance;
}

// Constructor
V2TIMFriendshipManagerImpl::V2TIMFriendshipManagerImpl() {
    V2TIM_LOG(kInfo, "V2TIMFriendshipManagerImpl initialized.");
    auto* tox = ToxManager::getInstance().getTox();
    if (tox) {
        tox_callback_friend_request(tox, friendRequestCallback);
    }
}

// Destructor
V2TIMFriendshipManagerImpl::~V2TIMFriendshipManagerImpl() {
    V2TIM_LOG(kInfo, "V2TIMFriendshipManagerImpl destroyed.");
}

// 好友请求回调处理
void V2TIMFriendshipManagerImpl::friendRequestCallback(Tox* tox, const uint8_t* public_key, const uint8_t* message, size_t length, void* user_data) {
    auto* impl = static_cast<V2TIMFriendshipManagerImpl*>(user_data);
    std::string msg(reinterpret_cast<const char*>(message), length);
    impl->handleFriendRequest(public_key, msg);
}

void V2TIMFriendshipManagerImpl::handleFriendRequest(const uint8_t* public_key, const std::string& message) {
    // 转换为十六进制字符串的Tox ID
    char tox_id[TOX_ADDRESS_SIZE * 2 + 1];
    tox_address_to_string_impl(public_key, tox_id);
    
    // 构造申请信息并通知监听器
    V2TIMFriendApplication application;
    application.userID = tox_id;
    application.addTime = time(nullptr);
    application.type = V2TIM_FRIEND_APPLICATION_COME_IN;

    TXV2TIMFriendApplicationVector vec;
    vec.PushBack(application);
    
    NotifyFriendApplicationListAdded(vec);
}

// --- Listener Management ---
void V2TIMFriendshipManagerImpl::AddFriendListener(V2TIMFriendshipListener* listener) {
    std::lock_guard<std::mutex> lock(listener_mutex_);
    if (listener) {
        // Avoid duplicates
        bool found = false;
        for (const auto& existing_listener : listeners_) {
            if (existing_listener == listener) {
                found = true;
                break;
            }
        }
        if (!found) {
            listeners_.push_back(listener);
        }
    }
}

void V2TIMFriendshipManagerImpl::RemoveFriendListener(V2TIMFriendshipListener* listener) {
    std::lock_guard<std::mutex> lock(listener_mutex_);
    if (listener) {
        listeners_.erase(std::remove(listeners_.begin(), listeners_.end(), listener), listeners_.end());
    }
}

// --- Helper for Reporting Not Implemented --- 
void V2TIMFriendshipManagerImpl::ReportNotImplemented(V2TIMCallback* callback) {
    if (callback) {
        callback->OnError(ERR_SDK_INTERFACE_NOT_SUPPORT, "Feature not supported in Tox wrapper");
    }
}

template <typename T>
void V2TIMFriendshipManagerImpl::ReportNotImplemented(V2TIMValueCallback<T>* callback) {
    if (callback) {
        callback->OnError(ERR_SDK_INTERFACE_NOT_SUPPORT, "Feature not supported in Tox wrapper");
    }
}

template <typename T>
void V2TIMFriendshipManagerImpl::ReportNotImplemented(V2TIMCompleteCallback<T>* callback) {
     if (callback) {
        callback->OnError(ERR_SDK_INTERFACE_NOT_SUPPORT, "Feature not supported in Tox wrapper");
    }
}

// --- Internal Notification Helpers ---
void V2TIMFriendshipManagerImpl::NotifyFriendApplicationListAdded(const V2TIMFriendApplicationVector& applicationList) {
    std::vector<V2TIMFriendshipListener*> listeners_copy;
    {
        std::lock_guard<std::mutex> lock(listener_mutex_);
        listeners_copy = listeners_; // Copy to allow modification during callback
    }
    V2TIM_LOG(kInfo, "Notifying {} FriendshipListeners about new applications.", listeners_copy.size());
    for (V2TIMFriendshipListener* listener : listeners_copy) {
        if (listener) {
            listener->OnFriendApplicationListAdded(applicationList);
        }
    }
}

void V2TIMFriendshipManagerImpl::NotifyFriendInfoChanged(const V2TIMFriendInfoResultVector& infoList) {
     std::vector<V2TIMFriendshipListener*> listeners_copy;
    {
        std::lock_guard<std::mutex> lock(listener_mutex_);
        listeners_copy = listeners_; 
    }
    V2TIM_LOG(kInfo, "Notifying {} FriendshipListeners about friend info changes.", listeners_copy.size());
    V2TIMFriendInfoVector friendInfoList; 
    for (const auto& result : infoList) {
        if (result.resultCode == 0) { 
            friendInfoList.PushBack(result.friendInfo);
        }
    }
    if (!friendInfoList.Empty()) { // Only notify if there are actual changes
        for (V2TIMFriendshipListener* listener : listeners_copy) {
            if (listener) {
                listener->OnFriendInfoChanged(friendInfoList);
            }
        }
    }
}

void V2TIMFriendshipManagerImpl::NotifyFriendListAdded(const V2TIMFriendInfoVector& infoList) {
     std::vector<V2TIMFriendshipListener*> listeners_copy;
    {
        std::lock_guard<std::mutex> lock(listener_mutex_);
        listeners_copy = listeners_; 
    }
    V2TIM_LOG(kInfo, "Notifying {} FriendshipListeners about friend list added.", listeners_copy.size());
    if (!infoList.Empty()) {
        for (V2TIMFriendshipListener* listener : listeners_copy) {
            if (listener) {
                listener->OnFriendListAdded(infoList);
            }
        }
    }
}
void V2TIMFriendshipManagerImpl::NotifyFriendListDeleted(const V2TIMStringVector& userIDList) {
     std::vector<V2TIMFriendshipListener*> listeners_copy;
    {
        std::lock_guard<std::mutex> lock(listener_mutex_);
        listeners_copy = listeners_; 
    }
    V2TIM_LOG(kInfo, "Notifying {} FriendshipListeners about friend list deleted.", listeners_copy.size());
     if (!userIDList.Empty()) {
        for (V2TIMFriendshipListener* listener : listeners_copy) {
            if (listener) {
                listener->OnFriendListDeleted(userIDList);
            }
        }
    }
}
void V2TIMFriendshipManagerImpl::NotifyBlackListAdded(const V2TIMFriendInfoVector& infoList) {
    std::vector<V2TIMFriendshipListener*> listeners_copy;
    {
        std::lock_guard<std::mutex> lock(listener_mutex_);
        listeners_copy = listeners_; 
    }
    V2TIM_LOG(kInfo, "Notifying {} FriendshipListeners about blacklist added.", listeners_copy.size());
     if (!infoList.Empty()) {
        for (V2TIMFriendshipListener* listener : listeners_copy) {
            if (listener) {
                listener->OnBlackListAdded(infoList);
            }
        }
    }
}
void V2TIMFriendshipManagerImpl::NotifyBlackListDeleted(const V2TIMStringVector& userIDList) {
     std::vector<V2TIMFriendshipListener*> listeners_copy;
    {
        std::lock_guard<std::mutex> lock(listener_mutex_);
        listeners_copy = listeners_; 
    }
    V2TIM_LOG(kInfo, "Notifying {} FriendshipListeners about blacklist deleted.", listeners_copy.size());
    if (!userIDList.Empty()) {
        for (V2TIMFriendshipListener* listener : listeners_copy) {
            if (listener) {
                listener->OnBlackListDeleted(userIDList);
            }
        }
    }
}


// --- Public API Implementations (Placeholders) ---
void V2TIMFriendshipManagerImpl::GetFriendList(V2TIMValueCallback<V2TIMFriendInfoVector>* callback) { 
    V2TIM_LOG(kInfo, "GetFriendList called");
    Tox* tox = ToxManager::getInstance().getTox();
    
    if (!tox) {
        V2TIM_LOG(kError, "GetFriendList: tox is null");
        if (callback) callback->OnError(ERR_SDK_NOT_INITIALIZED, "Tox not initialized");
        return;
    }
    
    V2TIMFriendInfoVector friendInfoVector;
    size_t friendCount = tox_self_get_friend_list_size(tox);
    
    if (friendCount > 0) {
        std::vector<uint32_t> friendList(friendCount);
        tox_self_get_friend_list(tox, friendList.data());
        
        for (uint32_t friendNumber : friendList) {
            V2TIMFriendInfo friendInfo;
            
            // Get friend public key (Tox ID) and use it as userID
            uint8_t friendId[TOX_PUBLIC_KEY_SIZE];
            if (tox_friend_get_public_key(tox, friendNumber, friendId, nullptr)) {
                // Manual conversion to hex string
                char hexId[TOX_PUBLIC_KEY_SIZE * 2 + 1];
                for (size_t i = 0; i < TOX_PUBLIC_KEY_SIZE; i++) {
                    sprintf(hexId + i * 2, "%02X", friendId[i]);
                }
                hexId[TOX_PUBLIC_KEY_SIZE * 2] = '\0';
                friendInfo.userID = hexId;
                
                // Get friend name if available
                size_t nameLen = tox_friend_get_name_size(tox, friendNumber, nullptr);
                if (nameLen > 0) {
                    std::vector<uint8_t> nameBytes(nameLen);
                    if (tox_friend_get_name(tox, friendNumber, nameBytes.data(), nullptr)) {
                        friendInfo.friendRemark = std::string(nameBytes.begin(), nameBytes.end());
                    }
                }
                
                // Get friend status message if available
                size_t statusLen = tox_friend_get_status_message_size(tox, friendNumber, nullptr);
                if (statusLen > 0) {
                    std::vector<uint8_t> statusBytes(statusLen);
                    if (tox_friend_get_status_message(tox, friendNumber, statusBytes.data(), nullptr)) {
                        std::string statusStr(statusBytes.begin(), statusBytes.end());
                        V2TIMBuffer statusBuffer(statusBytes.data(), statusLen);
                        friendInfo.friendCustomInfo.Insert("statusMessage", statusBuffer);
                    }
                }
                
                // Get online status
                TOX_CONNECTION connection = tox_friend_get_connection_status(tox, friendNumber, nullptr);
                friendInfo.userFullInfo.role = (connection != TOX_CONNECTION_NONE) ? V2TIM_USER_STATUS_ONLINE : V2TIM_USER_STATUS_OFFLINE;
                
                friendInfoVector.PushBack(friendInfo);
            }
        }
    }
    
    if (callback) callback->OnSuccess(friendInfoVector);
}

void V2TIMFriendshipManagerImpl::GetFriendsInfo(const V2TIMStringVector& userIDList, V2TIMValueCallback<V2TIMFriendInfoResultVector>* callback) { 
    V2TIM_LOG(kInfo, "GetFriendsInfo called");
     Tox* tox = ToxManager::getInstance().getTox();
    if (!tox) {
        ReportNotImplemented(callback); // Or specific error
        return;
    }
    
    V2TIMFriendInfoResultVector resultList;
    for (size_t i = 0; i < userIDList.Size(); i++) {
         auto userID = userIDList[i];
         V2TIMFriendInfoResult result;
         // 修改为使用正确的属性
         result.resultInfo = userID;
         uint8_t pubkey[TOX_PUBLIC_KEY_SIZE];
         if (tox_hex_to_bytes(userID.CString(), userID.Length(), pubkey, TOX_PUBLIC_KEY_SIZE)) {
             TOX_ERR_FRIEND_BY_PUBLIC_KEY err_find;
             uint32_t friend_num = tox_friend_by_public_key(tox, pubkey, &err_find);
             if (err_find == TOX_ERR_FRIEND_BY_PUBLIC_KEY_OK) {
                 // Found friend
                 V2TIMFriendInfo info;
                 info.userID = userID;
                 
                 // Get friend name
                 TOX_ERR_FRIEND_QUERY err_name;
                 size_t name_size = tox_friend_get_name_size(tox, friend_num, &err_name);
                 if (name_size > 0 && err_name == TOX_ERR_FRIEND_QUERY_OK) {
                     std::vector<uint8_t> name_buffer(name_size);
                     if (tox_friend_get_name(tox, friend_num, name_buffer.data(), &err_name) && err_name == TOX_ERR_FRIEND_QUERY_OK) {
                         // 修改为使用正确的属性
                         info.userFullInfo.nickName = V2TIMString(reinterpret_cast<const char*>(name_buffer.data()), name_size);
                     }
                  }
                  // TODO: Get remark, groups, etc.
                  
                  result.friendInfo = info;
                  result.resultCode = 0;
              } else {
                  result.resultCode = ERR_SVR_FRIENDSHIP_INVALID; // Not a friend
              }
          } else {
             result.resultCode = ERR_INVALID_PARAMETERS;
          }
         resultList.PushBack(result);
    }
    if (callback) callback->OnSuccess(resultList);
}
void V2TIMFriendshipManagerImpl::SetFriendInfo(const V2TIMFriendInfo& info, V2TIMCallback* callback) { 
    // TODO: Implement setting remark (requires local storage)
    V2TIM_LOG(kWarning, "SetFriendInfo: Not fully implemented (remark storage needed).");
    ReportNotImplemented(callback); 
}
void V2TIMFriendshipManagerImpl::SearchFriends(const V2TIMFriendSearchParam& searchParam, V2TIMValueCallback<V2TIMFriendInfoResultVector>* callback) { 
    V2TIM_LOG(kWarning, "SearchFriends: Not implemented.");
    ReportNotImplemented(callback);
} // Requires local search logic

void V2TIMFriendshipManagerImpl::AddFriend(const V2TIMFriendAddApplication& application, V2TIMValueCallback<V2TIMFriendOperationResult>* callback) {
    V2TIM_LOG(kInfo, "AddFriend called for user: {}", application.userID.CString());
    Tox* tox = ToxManager::getInstance().getTox();
    V2TIMFriendOperationResult result; // Declare outside if
    result.userID = application.userID;

    if (!tox) {
        result.resultCode=ERR_SDK_NOT_INITIALIZED;
        result.resultInfo = "Tox not initialized";
        if (callback) callback->OnSuccess(result); // Use OnSuccess as per V2TIM spec for this op
        return;
    }
    
    uint8_t public_key[TOX_PUBLIC_KEY_SIZE];
    if (!tox_hex_to_bytes(application.userID.CString(), application.userID.Length(), public_key, TOX_PUBLIC_KEY_SIZE)) {
        result.resultCode=ERR_INVALID_PARAMETERS;
        result.resultInfo = "Invalid UserID format";
        if (callback) callback->OnSuccess(result);
        return;
    }

    TOX_ERR_FRIEND_ADD err_add;
    tox_friend_add(tox, 
                   public_key, 
                   reinterpret_cast<const uint8_t*>(application.addWording.CString()),
                   application.addWording.Length(),
                   &err_add);

    if (err_add == TOX_ERR_FRIEND_ADD_OK) {
        result.resultCode = 0; 
        result.resultInfo = "Friend request sent";
    } else {
        // Map Tox error to V2TIM error code/info
        result.resultCode = ERR_SDK_FRIEND_ADD_FAILED; // General error
        switch(err_add) {
            case TOX_ERR_FRIEND_ADD_NULL: result.resultInfo = "Invalid arguments"; result.resultCode = ERR_INVALID_PARAMETERS; break;
            case TOX_ERR_FRIEND_ADD_TOO_LONG: result.resultInfo = "Request message too long"; result.resultCode = ERR_INVALID_PARAMETERS; break;
            case TOX_ERR_FRIEND_ADD_NO_MESSAGE: result.resultInfo = "Request message is empty"; result.resultCode = ERR_INVALID_PARAMETERS; break;
            case TOX_ERR_FRIEND_ADD_OWN_KEY: result.resultInfo = "Cannot add yourself"; result.resultCode = ERR_SDK_FRIEND_ADD_SELF; break;
            case TOX_ERR_FRIEND_ADD_ALREADY_SENT: result.resultInfo = "Friend request already sent"; result.resultCode = ERR_SDK_FRIEND_REQ_SENT; break;
            case TOX_ERR_FRIEND_ADD_BAD_CHECKSUM: result.resultInfo = "Invalid Tox ID checksum"; result.resultCode = ERR_INVALID_PARAMETERS; break;
            case TOX_ERR_FRIEND_ADD_SET_NEW_NOSPAM: result.resultInfo = "Invalid nospam value"; result.resultCode = ERR_INVALID_PARAMETERS; break;
            case TOX_ERR_FRIEND_ADD_MALLOC: result.resultInfo = "Memory allocation failed"; result.resultCode = ERR_OUT_OF_MEMORY; break;
            default: result.resultInfo = "Unknown error sending friend request"; break;
        }
         V2TIM_LOG(kError, "AddFriend failed: {} (Tox err: {})", result.resultInfo.CString(), err_add);
    }
    if (callback) callback->OnSuccess(result); 
}

void V2TIMFriendshipManagerImpl::DeleteFromFriendList(const V2TIMStringVector& userIDList, V2TIMFriendType deleteType, V2TIMValueCallback<V2TIMFriendOperationResultVector>* callback) { 
    V2TIM_LOG(kInfo, "DeleteFromFriendList called");
    Tox* tox = ToxManager::getInstance().getTox();
    if (!tox) {
        if (callback) callback->OnError(ERR_SDK_NOT_INITIALIZED, "Tox not initialized");
        return;
    }
    
    // Tox only offers "both" direction deletion.
    // It doesn't have a concept of single-sided friendship deletion.
    if (deleteType != V2TIM_FRIEND_TYPE_BOTH) {
        V2TIM_LOG(kWarning, "DeleteFromFriendList: Only V2TIM_FRIEND_TYPE_BOTH is supported by Tox.");
    }

    V2TIMFriendOperationResultVector results;
    for (size_t i = 0; i < userIDList.Size(); i++) {
        auto userID = userIDList[i];
        V2TIMFriendOperationResult result;
        result.userID = userID;
        uint8_t pubkey[TOX_PUBLIC_KEY_SIZE];
        // 使用我们自定义的tox_hex_to_bytes函数
        if (tox_hex_to_bytes(userID.CString(), userID.Length(), pubkey, TOX_PUBLIC_KEY_SIZE)) {
            TOX_ERR_FRIEND_BY_PUBLIC_KEY err_find;
            uint32_t friend_num = tox_friend_by_public_key(tox, pubkey, &err_find);
            if (err_find == TOX_ERR_FRIEND_BY_PUBLIC_KEY_OK) {
                TOX_ERR_FRIEND_DELETE err_del;
                bool success = tox_friend_delete(tox, friend_num, &err_del);
                if (success && err_del == TOX_ERR_FRIEND_DELETE_OK) {
                    result.resultCode = 0;
                    result.resultInfo = "Friend deleted successfully";
                    // TODO: Notify listener via NotifyFriendListDeleted
                } else {
                    result.resultCode = ERR_SDK_FRIEND_DELETE_FAILED;
                    result.resultInfo = "Failed to delete Tox friend";
                    V2TIM_LOG(kError, "Failed to delete friend {} (num {}), error: {}", userID.CString(), friend_num, err_del);
                }
            } else {
                 result.resultCode = ERR_SVR_FRIENDSHIP_INVALID; // Not found
                 result.resultInfo = "Friend not found";
            }
        } else {
            result.resultCode = ERR_INVALID_PARAMETERS;
            result.resultInfo = "Invalid UserID format";
        }
        results.PushBack(result);
    }
    if (callback) callback->OnSuccess(results);
}

void V2TIMFriendshipManagerImpl::CheckFriend(const V2TIMStringVector& userIDList, V2TIMFriendType checkType, V2TIMValueCallback<V2TIMFriendCheckResultVector>* callback) { 
    V2TIM_LOG(kInfo, "CheckFriend called");
    Tox* tox = ToxManager::getInstance().getTox();
    if (!tox) {
        if (callback) callback->OnError(ERR_SDK_NOT_INITIALIZED, "Tox not initialized");
        return;
    }
    
    // Tox doesn't have a built-in concept of double-sided/mutual friends
    // Therefore, we can only properly support checkType=V2TIM_FRIEND_TYPE_SINGLE.
    if (checkType != V2TIM_FRIEND_TYPE_SINGLE) {
         V2TIM_LOG(kWarning, "CheckFriend: Only V2TIM_FRIEND_TYPE_SINGLE is reliably supported by Tox.");
    }

    V2TIMFriendCheckResultVector results;
    for (size_t i = 0; i < userIDList.Size(); i++) {
        auto userID = userIDList[i];
        V2TIMFriendCheckResult result;
        result.userID = userID;
        result.resultCode = 0; // Assume success in checking, relationType holds the result
        uint8_t pubkey[TOX_PUBLIC_KEY_SIZE];
        // 使用我们自定义的tox_hex_to_bytes函数
        if (tox_hex_to_bytes(userID.CString(), userID.Length(), pubkey, TOX_PUBLIC_KEY_SIZE)) {
            TOX_ERR_FRIEND_BY_PUBLIC_KEY err_find;
            tox_friend_by_public_key(tox, pubkey, &err_find);
            if (err_find == TOX_ERR_FRIEND_BY_PUBLIC_KEY_OK) {
                // Is a friend
                result.relationType = V2TIM_FRIEND_RELATION_TYPE_BOTH_WAY;
            } else {
                // Not a friend
                result.relationType = V2TIM_FRIEND_RELATION_TYPE_NONE;
            }
        } else {
            result.resultCode = ERR_INVALID_PARAMETERS;
            result.relationType = V2TIM_FRIEND_RELATION_TYPE_NONE;
        }
        results.PushBack(result);
    }
    if (callback) callback->OnSuccess(results);
}

void V2TIMFriendshipManagerImpl::GetFriendApplicationList(V2TIMValueCallback<V2TIMFriendApplicationResult>* callback) { 
    V2TIM_LOG(kInfo, "GetFriendApplicationList called");
    Tox* tox = ToxManager::getInstance().getTox();
    
    if (!tox) {
        V2TIM_LOG(kError, "GetFriendApplicationList: tox is null");
        if (callback) callback->OnError(ERR_SDK_NOT_INITIALIZED, "Tox not initialized");
        return;
    }
    
    // In Tox, friend requests are not stored as a list that can be retrieved later
    // They are received as events and should be stored by the application
    // For now, we return an empty result
    
    V2TIMFriendApplicationResult result;
    // 设置空的application vector和其他必要字段
    
    if (callback) callback->OnSuccess(result);
}

void V2TIMFriendshipManagerImpl::AcceptFriendApplication(const V2TIMFriendApplication& application, V2TIMFriendAcceptType acceptType, V2TIMValueCallback<V2TIMFriendOperationResult>* callback) { 
     V2TIM_LOG(kInfo, "AcceptFriendApplication called for user: {}", application.userID.CString());
     Tox* tox = ToxManager::getInstance().getTox();
    V2TIMFriendOperationResult result; // Declare outside if
    result.userID = application.userID;
    
    if (!tox) {
        result.resultCode = ERR_SDK_NOT_INITIALIZED;
        result.resultInfo = "Tox not initialized";
        if (callback) callback->OnSuccess(result);
        return;
    }
    
    uint8_t public_key[TOX_PUBLIC_KEY_SIZE];
    // 使用我们自定义的tox_hex_to_bytes函数
    if (!tox_hex_to_bytes(application.userID.CString(), application.userID.Length(), public_key, TOX_PUBLIC_KEY_SIZE)) {
         result.resultCode = ERR_INVALID_PARAMETERS;
         result.resultInfo = "Invalid UserID format";
         if (callback) callback->OnSuccess(result);
         return;
    }
        
    // NOTE: V2TIMFriendAcceptType is ignored, Tox just adds the friend back.
    if (acceptType != V2TIM_FRIEND_ACCEPT_AGREE_AND_ADD) {
         V2TIM_LOG(kWarning, "AcceptFriendApplication: Only V2TIM_FRIEND_ACCEPT_AGREE_AND_ADD behavior is supported.");
    }

    TOX_ERR_FRIEND_ADD err_add;
    tox_friend_add_norequest(tox, public_key, &err_add);
    
    if (err_add == TOX_ERR_FRIEND_ADD_OK) {
        result.resultCode = 0;
        result.resultInfo = "Friend added successfully";
        // TODO: Notify OnFriendListAdded?
    } else {
        result.resultCode = ERR_SDK_FRIEND_ADD_FAILED; 
        result.resultInfo = "Failed to add Tox friend"; // TODO: Map specific errors
        V2TIM_LOG(kError, "AcceptFriendApplication failed: {} (Tox err: {})", result.resultInfo.CString(), err_add);
    }
     if (callback) callback->OnSuccess(result);
}
void V2TIMFriendshipManagerImpl::AcceptFriendApplication(const V2TIMFriendApplication& application, V2TIMFriendAcceptType acceptType, const V2TIMString& remark, V2TIMValueCallback<V2TIMFriendOperationResult>* callback) { 
    AcceptFriendApplication(application, acceptType, callback);
    if (!remark.Empty()) {
        V2TIM_LOG(kWarning, "AcceptFriendApplication with remark: Remarks are not supported by Tox.");
    }
}
void V2TIMFriendshipManagerImpl::RefuseFriendApplication(const V2TIMFriendApplication& application, V2TIMValueCallback<V2TIMFriendOperationResult>* callback) { 
    V2TIM_LOG(kInfo, "RefuseFriendApplication called");
    
    V2TIMFriendOperationResult result;
    result.resultCode = 0; // Success (ish)
    result.resultInfo = "Refusing not implemented, but not needed in Tox";
    result.userID = application.userID;
    
    V2TIM_LOG(kWarning, "RefuseFriendApplication: Tox doesn't support actively rejecting friend requests.");
    if (callback) callback->OnSuccess(result);
}
void V2TIMFriendshipManagerImpl::DeleteFriendApplication(const V2TIMFriendApplication& application, V2TIMCallback* callback) {
    V2TIM_LOG(kInfo, "DeleteFriendApplication called");
    
    V2TIM_LOG(kWarning, "DeleteFriendApplication: Friend application deletion is a client-side concept.");
    if (callback) callback->OnSuccess();
}
void V2TIMFriendshipManagerImpl::SetFriendApplicationRead(V2TIMCallback* callback) { 
    V2TIM_LOG(kInfo, "SetFriendApplicationRead called");
    V2TIM_LOG(kWarning, "SetFriendApplicationRead: Friend application read status is a client-side concept.");
    if (callback) callback->OnSuccess();
}
void V2TIMFriendshipManagerImpl::AddToBlackList(const V2TIMStringVector& userIDList, V2TIMValueCallback<V2TIMFriendOperationResultVector>* callback) {
    V2TIM_LOG(kWarning, "AddToBlackList: Blacklist not implemented in Tox");
    ReportNotImplemented(callback);
}
void V2TIMFriendshipManagerImpl::DeleteFromBlackList(const V2TIMStringVector& userIDList, V2TIMValueCallback<V2TIMFriendOperationResultVector>* callback) {
    V2TIM_LOG(kWarning, "DeleteFromBlackList: Blacklist not implemented in Tox");
    ReportNotImplemented(callback);
}
void V2TIMFriendshipManagerImpl::GetBlackList(V2TIMValueCallback<V2TIMFriendInfoVector>* callback) {
    V2TIM_LOG(kWarning, "GetBlackList: Blacklist not implemented in Tox");
    ReportNotImplemented(callback);
}
void V2TIMFriendshipManagerImpl::CreateFriendGroup(const V2TIMString& groupName, const V2TIMStringVector& userIDList, V2TIMValueCallback<V2TIMFriendOperationResultVector>* callback) { 
    V2TIM_LOG(kInfo, "CreateFriendGroup called with name: %s", groupName.CString());
    
    // Tox doesn't support friend groups natively
    // We return success for the API but with a note that this is not supported
    V2TIMFriendOperationResultVector resultVector;
    
    for (size_t i = 0; i < userIDList.Size(); i++) {
        V2TIMFriendOperationResult result;
        result.userID = userIDList[i];
        result.resultCode = 0; // Success code
        result.resultInfo = "Friend groups not supported in Tox, but operation recorded";
        resultVector.PushBack(result);
    }
    
    if (callback) callback->OnSuccess(resultVector);
}
void V2TIMFriendshipManagerImpl::GetFriendGroups(const V2TIMStringVector& groupNameList, V2TIMValueCallback<V2TIMFriendGroupVector>* callback) { 
    V2TIM_LOG(kInfo, "GetFriendGroups called");
    
    // Tox doesn't support friend groups, so we return an empty list
    V2TIMFriendGroupVector groupVector;
    
    if (callback) callback->OnSuccess(groupVector);
}
void V2TIMFriendshipManagerImpl::DeleteFriendsFromFriendGroup(const V2TIMString& groupName, const V2TIMStringVector& userIDList, V2TIMValueCallback<V2TIMFriendOperationResultVector>* callback) {
    V2TIM_LOG(kInfo, "DeleteFriendsFromFriendGroup called with group %s and %zu users", groupName.CString(), userIDList.Size());
    
    // Tox doesn't support friend groups, but we'll return success with empty results
    V2TIMFriendOperationResultVector results;
    if (callback) {
        callback->OnSuccess(results);
    }
}

// --- Official Account Management ---
void V2TIMFriendshipManagerImpl::SubscribeOfficialAccount(const V2TIMString& officialAccountID, V2TIMCallback* callback) {
    // V2TIM_LOG(kWarning, "SubscribeOfficialAccount: Not implemented in tim2tox.");
    ReportNotImplemented(callback);
}

void V2TIMFriendshipManagerImpl::UnsubscribeOfficialAccount(const V2TIMString& officialAccountID, V2TIMCallback* callback) {
    // V2TIM_LOG(kWarning, "UnsubscribeOfficialAccount: Not implemented in tim2tox.");
    ReportNotImplemented(callback);
}

void V2TIMFriendshipManagerImpl::GetOfficialAccountsInfo(const V2TIMStringVector& officialAccountIDList, 
                                                       V2TIMValueCallback<V2TIMOfficialAccountInfoResultVector>* callback) {
    // V2TIM_LOG(kWarning, "GetOfficialAccountsInfo: Not implemented in tim2tox.");
    ReportNotImplemented(callback);
}

// --- Follow Management ---
void V2TIMFriendshipManagerImpl::FollowUser(const V2TIMStringVector& userIDList, 
                                          V2TIMValueCallback<V2TIMFollowOperationResultVector>* callback) {
    // V2TIM_LOG(kWarning, "FollowUser: Not implemented in tim2tox.");
    ReportNotImplemented(callback);
}

void V2TIMFriendshipManagerImpl::UnfollowUser(const V2TIMStringVector& userIDList, 
                                            V2TIMValueCallback<V2TIMFollowOperationResultVector>* callback) {
    // V2TIM_LOG(kWarning, "UnfollowUser: Not implemented in tim2tox.");
    ReportNotImplemented(callback);
}

void V2TIMFriendshipManagerImpl::GetMyFollowingList(const V2TIMString& nextCursor, 
                                                  V2TIMValueCallback<V2TIMUserInfoResult>* callback) {
    // V2TIM_LOG(kWarning, "GetMyFollowingList: Not implemented in tim2tox.");
    ReportNotImplemented(callback);
}

void V2TIMFriendshipManagerImpl::GetMyFollowersList(const V2TIMString& nextCursor, 
                                                  V2TIMValueCallback<V2TIMUserInfoResult>* callback) {
    // V2TIM_LOG(kWarning, "GetMyFollowersList: Not implemented in tim2tox.");
    ReportNotImplemented(callback);
}

void V2TIMFriendshipManagerImpl::GetMutualFollowersList(const V2TIMString& nextCursor, 
                                                      V2TIMValueCallback<V2TIMUserInfoResult>* callback) {
    // V2TIM_LOG(kWarning, "GetMutualFollowersList: Not implemented in tim2tox.");
    ReportNotImplemented(callback);
}

void V2TIMFriendshipManagerImpl::GetUserFollowInfo(const V2TIMStringVector& userIDList, 
                                                 V2TIMValueCallback<V2TIMFollowInfoVector>* callback) {
    // V2TIM_LOG(kWarning, "GetUserFollowInfo: Not implemented in tim2tox.");
    ReportNotImplemented(callback);
}

void V2TIMFriendshipManagerImpl::CheckFollowType(const V2TIMStringVector& userIDList, 
                                               V2TIMValueCallback<V2TIMFollowTypeCheckResultVector>* callback) {
    // V2TIM_LOG(kWarning, "CheckFollowType: Not implemented in tim2tox.");
    ReportNotImplemented(callback);
}
