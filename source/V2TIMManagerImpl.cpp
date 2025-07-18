#include "V2TIMManagerImpl.h"
#include "V2TIMUtils.h"
#include <V2TIMErrorCode.h>
#include "ToxManager.h"
#include "tox.h"
#include "ToxUtil.h"
#include <cstdlib>
#include <ctime>
#include <cstdio>
#include <unordered_map>
#include <chrono>
#include "V2TIMLog.h" // Updated include
#include <vector> // For buffer operations

// Include implementation headers for sub-managers
#include "V2TIMMessageManagerImpl.h"
#include "V2TIMGroupManagerImpl.h"
#include "V2TIMCommunityManagerImpl.h"
#include "V2TIMConversationManagerImpl.h"
#include "V2TIMFriendshipManagerImpl.h"
#include "V2TIMSignalingManagerImpl.h"

// Static method to get singleton instance
V2TIMManagerImpl* V2TIMManagerImpl::GetInstance() {
    static V2TIMManagerImpl instance;
    return &instance;
}

// Constructor (if not existing, add it)
V2TIMManagerImpl::V2TIMManagerImpl() : running_(true) {
    // Initialize random seed if not done elsewhere
    std::srand(std::time(nullptr));
}

// Destructor (if not existing, add it)
V2TIMManagerImpl::~V2TIMManagerImpl() {
    // Ensure thread is joined if running
    if (running_) {
        UnInitSDK();
    }
}

// SDK Listener methods
void V2TIMManagerImpl::AddSDKListener(V2TIMSDKListener* listener) {
    std::lock_guard<std::mutex> lock(mutex_);
    sdk_listeners_.insert(listener);
}

void V2TIMManagerImpl::RemoveSDKListener(V2TIMSDKListener* listener) {
    std::lock_guard<std::mutex> lock(mutex_);
    sdk_listeners_.erase(listener);
}

// SDK initialization and shutdown
bool V2TIMManagerImpl::InitSDK(uint32_t sdkAppID, const V2TIMSDKConfig& config) {
    ToxManager::getInstance().initialize();
    Tox* tox = ToxManager::getInstance().getTox();
    if (!tox) {
        // Log error
        return false;
    }

    // --- Register Callbacks with ToxManager --- 
    ToxManager::getInstance().setGroupMessageCallback(
        [this](uint32_t conference_number, uint32_t peer_number, TOX_MESSAGE_TYPE type, const uint8_t* message, size_t length) {
            this->HandleGroupMessage(conference_number, peer_number, type, message, length);
        }
    );
    ToxManager::getInstance().setFriendMessageCallback(
        [this](uint32_t friend_number, TOX_MESSAGE_TYPE type, const uint8_t* message, size_t length) {
            this->HandleFriendMessage(friend_number, type, message, length);
        }
    );
    ToxManager::getInstance().setSelfConnectionStatusCallback(
        [this](TOX_CONNECTION connection_status) {
            this->HandleSelfConnectionStatus(connection_status);
        }
    );
    ToxManager::getInstance().setFriendRequestCallback(
        [this](const uint8_t* public_key, const uint8_t* message, size_t length) {
            this->HandleFriendRequest(public_key, message, length);
        }
    );
    ToxManager::getInstance().setFriendNameCallback(
        [this](uint32_t friend_number, const uint8_t* name, size_t length) {
            this->HandleFriendName(friend_number, name, length);
        }
    );
    ToxManager::getInstance().setFriendStatusMessageCallback(
        [this](uint32_t friend_number, const uint8_t* message, size_t length) {
            this->HandleFriendStatusMessage(friend_number, message, length);
        }
    );
    ToxManager::getInstance().setFriendStatusCallback(
        [this](uint32_t friend_number, TOX_USER_STATUS status) {
            this->HandleFriendStatus(friend_number, status);
        }
    );
    ToxManager::getInstance().setGroupTitleCallback(
        [this](uint32_t conference_number, uint32_t peer_number, const uint8_t* title, size_t length) {
            this->HandleGroupTitle(conference_number, peer_number, title, length);
        }
    );
    ToxManager::getInstance().setGroupPeerNameCallback(
        [this](uint32_t conference_number, uint32_t peer_number, const uint8_t* name, size_t length) {
            this->HandleGroupPeerName(conference_number, peer_number, name, length);
        }
    );
    ToxManager::getInstance().setGroupPeerListChangedCallback(
        [this](uint32_t conference_number) {
            this->HandleGroupPeerListChanged(conference_number);
        }
    );
    // TODO: Register handlers for other callbacks (read receipts, file transfer, etc.)

    // Start the Tox event loop thread
    running_ = true; // Ensure running flag is set before starting thread
    event_thread_ = std::thread([this, tox] {
        while (running_) {
             // Iterate Tox events. Pass 'this' as user_data directly
            tox_iterate(tox, this); // Pass 'this' directly instead of relying on tox_self_set_user_data
            
            // Get iteration interval and sleep
            uint32_t interval = tox_iteration_interval(tox);
            std::this_thread::sleep_for(std::chrono::milliseconds(interval));
        }
    });

    return true;
}

void V2TIMManagerImpl::UnInitSDK() {
    running_ = false;
    if (event_thread_.joinable()) event_thread_.join();
    ToxManager::getInstance().shutdown();
}

// SDK Information
V2TIMString V2TIMManagerImpl::GetVersion() {
    return "1.0.0";
}

int64_t V2TIMManagerImpl::GetServerTime() {
    return std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
}

// User Authentication
void V2TIMManagerImpl::Login(const V2TIMString& userID, const V2TIMString& userSig, V2TIMCallback* callback) {
    logged_in_user_ = userID;
    const char* bootstrap_node = "tox.ngc.zone";
    uint16_t port = 33445;
    uint8_t public_key[] = { /* Public key here */ };
    tox_bootstrap(ToxManager::getInstance().getTox(), bootstrap_node, port, public_key, nullptr);

    if (callback) callback->OnSuccess();
}

void V2TIMManagerImpl::Logout(V2TIMCallback* callback) {
    logged_in_user_ = "";
    if (callback) callback->OnSuccess();
}

V2TIMString V2TIMManagerImpl::GetLoginUser() {
    return logged_in_user_;
}

V2TIMLoginStatus V2TIMManagerImpl::GetLoginStatus() {
    return logged_in_user_.Empty() ? V2TIMLoginStatus::V2TIM_STATUS_LOGOUT : V2TIMLoginStatus::V2TIM_STATUS_LOGINED;
}

// Messaging
void V2TIMManagerImpl::AddSimpleMsgListener(V2TIMSimpleMsgListener* listener) {
    std::lock_guard<std::mutex> lock(mutex_);
    simple_msg_listeners_.insert(listener);
}

void V2TIMManagerImpl::RemoveSimpleMsgListener(V2TIMSimpleMsgListener* listener) {
    std::lock_guard<std::mutex> lock(mutex_);
    simple_msg_listeners_.erase(listener);
}

V2TIMString V2TIMManagerImpl::SendC2CTextMessage(
    const V2TIMString& text, 
    const V2TIMString& userID,
    V2TIMSendCallback* callback) {
    // ===================================================================
    // Step 1: Validate parameters
    // ===================================================================
    if (text.Empty()) {
        if (callback) {
            callback->OnError(ERR_INVALID_PARAMETERS, 
                            "Message text cannot be empty");
        }
        return "";
    }

    if (userID.Empty()) {
        if (callback) {
            callback->OnError(ERR_INVALID_PARAMETERS, 
                            "UserID cannot be empty");
        }
        return "";
    }

    // ===================================================================
    // Step 2: Convert UserID (assuming hex public key) to Tox public key bytes
    // ===================================================================
    uint8_t public_key[TOX_PUBLIC_KEY_SIZE] = {0};
    
    // Assuming V2TIMUtils has a static HexToBytes method.
    // If V2TIMUtils is not a class/namespace with static methods, this needs adjustment.
    // bool convert_result = V2TIMUtils::HexToBytes(userID.CString(), public_key, TOX_PUBLIC_KEY_SIZE);
    // Using direct tox function for now, assuming userID *is* the hex address string
    bool convert_result = ToxUtil::tox_hex_to_bytes(userID.CString(), userID.Length(), public_key, TOX_PUBLIC_KEY_SIZE);

    if (!convert_result) {
        if (callback) {
            callback->OnError(ERR_INVALID_PARAMETERS,
                            "Invalid userID format (must be hex Tox ID)");
        }
        return "";
    }

    // ===================================================================
    // Step 3: Look up friend number by public key
    // ===================================================================
    std::unique_lock<std::mutex> lock(mutex_); // Use the main mutex
    Tox* tox = ToxManager::getInstance().getTox();
    if (!tox) {
         if (callback) callback->OnError(ERR_SDK_NOT_INITIALIZED, "Tox instance not available");
         return "";
    }
    
    TOX_ERR_FRIEND_BY_PUBLIC_KEY find_err;
    uint32_t friend_number = tox_friend_by_public_key(
        tox, 
        public_key, 
        &find_err
    );

    if (find_err != TOX_ERR_FRIEND_BY_PUBLIC_KEY_OK) {
        lock.unlock();
        if (callback) {
            // Use appropriate error code based on V2TIMErrorCode.h
            int v2_err = (find_err == TOX_ERR_FRIEND_BY_PUBLIC_KEY_NOT_FOUND)
                       ? ERR_SVR_FRIENDSHIP_ACCOUNT_NOT_FOUND
                       : ERR_INVALID_PARAMETERS;
            const char* err_msg = (find_err == TOX_ERR_FRIEND_BY_PUBLIC_KEY_NOT_FOUND)
                                ? "Target user is not your friend"
                                : "Friend lookup failed";
            callback->OnError(v2_err, err_msg);
        }
        return "";
    }

    // ===================================================================
    // Step 4: Send the message
    // ===================================================================
    // Generate a unique V2TIM message ID
    uint64_t timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                            std::chrono::system_clock::now().time_since_epoch())
                            .count();
    uint32_t random_part = std::rand();
    char msg_id_buffer[64];
    snprintf(msg_id_buffer, sizeof(msg_id_buffer), "c%llu-%u", timestamp, random_part); // Prefix with 'c' for C2C
    V2TIMString msg_id = msg_id_buffer;

    const size_t max_msg_len = TOX_MAX_MESSAGE_LENGTH;
    if (text.Length() > max_msg_len) {
        lock.unlock();
        if (callback) {
            // Corrected closing parenthesis
            callback->OnError(ERR_SDK_MSG_BODY_SIZE_LIMIT,
                              "Message exceeds max length");
        }
        return "";
    }

    TOX_ERR_FRIEND_SEND_MESSAGE send_err;
    // Note: message_return_id is the internal tox id, might be useful for receipts
    // Commented out to avoid unused variable warning
    /*uint32_t message_return_id =*/ tox_friend_send_message(
        tox,
        friend_number,
        TOX_MESSAGE_TYPE_NORMAL,
        reinterpret_cast<const uint8_t*>(text.CString()),
        text.Length(),
        &send_err
    );

    lock.unlock();

    // ===================================================================
    // Step 5: Handle send result
    // ===================================================================
    if (send_err == TOX_ERR_FRIEND_SEND_MESSAGE_OK) {
        // TODO: Implement message status tracking/receipts using message_return_id if needed
        if (callback) {
            // Create a V2TIMMessage from the message ID
            V2TIMMessage resultMsg;
            resultMsg.msgID = msg_id;
            // Add a text elem to the message
            V2TIMTextElem* textElem = new V2TIMTextElem();
            textElem->text = text;
            resultMsg.elemList.PushBack(textElem);
            // Set basic message properties
            resultMsg.timestamp = std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
            resultMsg.status = V2TIM_MSG_STATUS_SEND_SUCC;
            resultMsg.userID = userID;
            resultMsg.sender = ToxManager::getInstance().getAddress();
            
            // Pass the V2TIMMessage to the callback
            callback->OnSuccess(resultMsg);
        }
        return msg_id;
    }
    else {
         // Map Tox error to V2TIM error code
        int v2_err_code = ERR_INVALID_PARAMETERS; // Default
        const char* v2_err_msg = "Unknown Tox error during send";

        switch (send_err) {
            case TOX_ERR_FRIEND_SEND_MESSAGE_FRIEND_NOT_FOUND:
                 v2_err_code = ERR_SVR_FRIENDSHIP_ACCOUNT_NOT_FOUND; // Should not happen if lookup succeeded
                 v2_err_msg = "Friend not found internally";
                 break;
            case TOX_ERR_FRIEND_SEND_MESSAGE_FRIEND_NOT_CONNECTED:
                 v2_err_code = ERR_SDK_NET_DISCONNECT;
                 v2_err_msg = "Friend not connected";
                 break;
            case TOX_ERR_FRIEND_SEND_MESSAGE_SENDQ:
                 v2_err_code = ERR_INVALID_PARAMETERS; // Or ERR_SDK_NET_REQ_COUNT_LIMIT?
                 v2_err_msg = "Send queue full";
                 break;
             case TOX_ERR_FRIEND_SEND_MESSAGE_TOO_LONG:
                 v2_err_code = ERR_SDK_MSG_BODY_SIZE_LIMIT;
                 v2_err_msg = "Message too long (Tox limit)";
                 break;
             case TOX_ERR_FRIEND_SEND_MESSAGE_EMPTY:
                  v2_err_code = ERR_INVALID_PARAMETERS;
                  v2_err_msg = "Message text cannot be empty (Tox check)";
                  break;
            default:
                  v2_err_code = ERR_INVALID_PARAMETERS;
                  v2_err_msg = "Unknown Tox error during send";
                  break;
            // Add other specific Tox errors if needed
        }
        if (callback) callback->OnError(v2_err_code, v2_err_msg);
        return "";
    }
}

// Send C2C custom message
V2TIMString V2TIMManagerImpl::SendC2CCustomMessage(const V2TIMBuffer& customData, const V2TIMString& userID, V2TIMSendCallback* callback) {
    // TODO: Implement C2C Custom Message Sending
    if (callback) callback->OnError(ERR_SDK_INTERFACE_NOT_SUPPORT, "Not implemented yet");
    return "";
}

// Send group text message
V2TIMString V2TIMManagerImpl::SendGroupTextMessage(const V2TIMString& text, const V2TIMString& groupID, V2TIMMessagePriority priority, V2TIMSendCallback* callback) {
    // ===================================================================
    // Step 1: Validate parameters
    // ===================================================================
    if (text.Empty()) {
        if (callback) callback->OnError(ERR_INVALID_PARAMETERS, "Message text cannot be empty");
        return "";
    }
    if (groupID.Empty()) {
        if (callback) callback->OnError(ERR_INVALID_PARAMETERS, "Group ID cannot be empty");
        return "";
    }

    // Ignore priority for now, as Tox core doesn't directly support it for group messages

    // ===================================================================
    // Step 2: Find the conference number for the group ID
    // ===================================================================
    uint32_t conference_number;
    {
        std::lock_guard<std::mutex> lock(mutex_); // Protect access to the map
        // TODO: Need to populate group_id_to_conference_number_ map in CreateGroup/JoinGroup/etc.
        // Ensure group_id_to_conference_number_ is declared in V2TIMManagerImpl.h
        auto it = group_id_to_conference_number_.find(groupID);
        if (it == group_id_to_conference_number_.end()) {
            if (callback) callback->OnError(ERR_INVALID_PARAMETERS, "Group not found or user not in group");
            return "";
        }
        conference_number = it->second;
    }

    // ===================================================================
    // Step 3: Check message length against Tox limit
    // ===================================================================
    // Note: V2TIM API implies single message. We'll check against the general Tox limit.
    const size_t max_msg_len = TOX_MAX_MESSAGE_LENGTH; // Use general constant
    if (text.Length() > max_msg_len) {
       if (callback) callback->OnError(ERR_SDK_MSG_BODY_SIZE_LIMIT, "Message exceeds max length");
       return "";
    }

    // ===================================================================
    // Step 4: Send the message using ToxManager
    // ===================================================================
    // Generate a unique message ID (e.g., timestamp + random)
    // Note: This ID is local; Tox doesn't provide a message ID on send for groups.
    uint64_t timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                            std::chrono::system_clock::now().time_since_epoch())
                            .count();
    uint32_t random_part = std::rand();
    char msg_id_buffer[64];
    snprintf(msg_id_buffer, sizeof(msg_id_buffer), "g%llu-%u", timestamp, random_part); // Prefix 'g' for group
    V2TIMString msgID = msg_id_buffer;


    TOX_ERR_CONFERENCE_SEND_MESSAGE send_err;
    bool success = ToxManager::getInstance().groupSendMessage(
        conference_number,
        TOX_MESSAGE_TYPE_NORMAL, // For text messages
        reinterpret_cast<const uint8_t*>(text.CString()),
        text.Length(),
        &send_err);

    // ===================================================================
    // Step 5: Handle the result
    // ===================================================================
    if (success) {
        // Tox group send is fire-and-forget, success means it was queued.
        if (callback) {
             // Create a V2TIMMessage
             V2TIMMessage resultMsg;
             resultMsg.msgID = msgID;
             
             // Add text element
             V2TIMTextElem* textElem = new V2TIMTextElem();
             textElem->text = text;
             resultMsg.elemList.PushBack(textElem);
             
             // Set basic message properties
             resultMsg.timestamp = std::chrono::duration_cast<std::chrono::seconds>(
                 std::chrono::system_clock::now().time_since_epoch()).count();
             resultMsg.status = V2TIM_MSG_STATUS_SEND_SUCC;
             resultMsg.groupID = groupID;
             resultMsg.sender = ToxManager::getInstance().getAddress();
             
             // Pass the message to the callback
             callback->OnSuccess(resultMsg);
        }
        return msgID;
    } else {
        // Map Tox error to V2TIM error code
        int v2_err_code = ERR_INVALID_PARAMETERS; // Default error
        const char* v2_err_msg = "Failed to send group message";
        switch (send_err) {
            // case TOX_ERR_CONFERENCE_SEND_MESSAGE_OK: // Should not happen if success is false
            //     break;
            case TOX_ERR_CONFERENCE_SEND_MESSAGE_CONFERENCE_NOT_FOUND:
                 // This implies the conference_number became invalid between lookup and send,
                 // or the ToxManager instance disappeared.
                 v2_err_code = ERR_INVALID_PARAMETERS; // Or perhaps ERR_SDK_NOT_INITIALIZED
                 v2_err_msg = "Internal error: Conference number invalid or Tox instance gone";
                 break;
            case TOX_ERR_CONFERENCE_SEND_MESSAGE_TOO_LONG:
                 v2_err_code = ERR_SDK_MSG_BODY_SIZE_LIMIT;
                 v2_err_msg = "Message too long";
                 break;
            case TOX_ERR_CONFERENCE_SEND_MESSAGE_NO_CONNECTION:
                  v2_err_code = ERR_SDK_NET_DISCONNECT; // Map to general network error
                  v2_err_msg = "Not connected to the group chat network";
                  break;
            case TOX_ERR_CONFERENCE_SEND_MESSAGE_FAIL_SEND:
                  v2_err_code = ERR_INVALID_PARAMETERS;
                  v2_err_msg = "Failed to send message to group";
                  break;
            default:
                  v2_err_code = ERR_INVALID_PARAMETERS;
                  v2_err_msg = "Unknown error sending group message";
                  break;
             // Add more cases as needed based on toxcore version/errors
        }
        if (callback) callback->OnError(v2_err_code, v2_err_msg);
        return "";
    }
}

// Send group custom message
V2TIMString V2TIMManagerImpl::SendGroupCustomMessage(const V2TIMBuffer& customData, const V2TIMString& groupID, V2TIMMessagePriority priority, V2TIMSendCallback* callback) {
    // TODO: Implement Group Custom Message Sending
    if (callback) callback->OnError(ERR_SDK_INTERFACE_NOT_SUPPORT, "Not implemented yet");
    return "";
}

// Group Management
void V2TIMManagerImpl::AddGroupListener(V2TIMGroupListener* listener) {
    std::lock_guard<std::mutex> lock(mutex_);
    group_listeners_.insert(listener);
}

void V2TIMManagerImpl::RemoveGroupListener(V2TIMGroupListener* listener) {
    std::lock_guard<std::mutex> lock(mutex_);
    group_listeners_.erase(listener);
}

void V2TIMManagerImpl::CreateGroup(const V2TIMString& groupType, const V2TIMString& groupID, const V2TIMString& groupName, V2TIMValueCallback<V2TIMString>* callback) {
    Tox* tox = ToxManager::getInstance().getTox();
    if (!tox) {
        V2TIM_LOG(kError, "CreateGroup failed: Tox not initialized");
        if (callback) callback->OnError(ERR_SDK_NOT_INITIALIZED, "Tox not initialized");
        return;
    }

    // Determine conference type (currently Tox only supports TEXT type implicitly)
    // TODO: Map groupType ("Work", "Public", "Meeting", etc.) to Tox capabilities if needed in the future.
    // For now, we ignore groupType and create a standard text conference.

    TOX_ERR_CONFERENCE_NEW err_new;
    uint32_t conference_number = tox_conference_new(tox, &err_new);

    if (err_new != TOX_ERR_CONFERENCE_NEW_OK) {
        // Map Tox error to V2TIM error
        int v2_err = ERR_INVALID_PARAMETERS;
        const char* v2_msg = "Failed to create Tox conference";
        // Add specific error mapping if needed based on TOX_ERR_CONFERENCE_NEW values
        if (callback) callback->OnError(v2_err, v2_msg);
        return;
    }

    // Determine the final Group ID
    V2TIMString finalGroupID = groupID;
    if (finalGroupID.Empty()) {
        // Generate a unique ID if none provided. Using conference number as string.
        char generated_id_buf[20]; // Should be enough for uint32_t
        snprintf(generated_id_buf, sizeof(generated_id_buf), "tox_%u", conference_number);
        finalGroupID = generated_id_buf;
    }
    // TODO: Validate provided groupID format/uniqueness if necessary, especially for specific groupTypes like "Community".

    // Set group title if provided
    if (!groupName.Empty()) {
        TOX_ERR_CONFERENCE_TITLE err_title;
        tox_conference_set_title(tox,
                                 conference_number,
                                 reinterpret_cast<const uint8_t*>(groupName.CString()),
                                 groupName.Length(),
                                 &err_title);
        if (err_title != TOX_ERR_CONFERENCE_TITLE_OK) {
            V2TIM_LOG(kWarning, "Failed to set group title for group {} (conf {})", finalGroupID.CString(), conference_number);
        }
    }

    // Store the mapping (both ways)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        // Check if groupID already exists? V2TIM allows providing ID, Tox assigns number.
        // If finalGroupID exists in group_id_to_conference_number_, maybe error? Or overwrite?
        // For now, assume overwrite is fine or provided IDs are unique.
        group_id_to_conference_number_[finalGroupID] = conference_number;
        conference_number_to_group_id_[conference_number] = finalGroupID;
    }

    V2TIM_LOG(kInfo, "Created group {} (conf {})", finalGroupID.CString(), conference_number);
    if (callback) callback->OnSuccess(finalGroupID);
}

void V2TIMManagerImpl::JoinGroup(const V2TIMString& groupID, const V2TIMString& message, V2TIMCallback* callback) {
    // TODO: Implement group joining.
    // Current V2TIM API doesn't directly map to tox_conference_join which requires
    // friend_number (inviter) and a cookie received via tox_conference_invite.
    // A full implementation would require:
    // 1. Handling tox_conference_invite on the inviting side.
    // 2. Handling the onGroupInvite callback on the receiving side to store the cookie.
    // 3. Looking up the stored cookie here based on groupID (or modifying API).
    // 4. Calling tox_conference_join with the correct friend_number and cookie.
    // 5. Populating the group_id_to_conference_number_ and conference_number_to_group_id_ maps on success.

    V2TIM_LOG(kError, "JoinGroup is not fully implemented due to Tox invite mechanism requirements.");
    if (callback) callback->OnError(ERR_SDK_INTERFACE_NOT_SUPPORT, 
        "Joining Tox groups requires an invitation mechanism not yet implemented.");
}

void V2TIMManagerImpl::QuitGroup(const V2TIMString& groupID, V2TIMCallback* callback) {
    // Simulate quitting the group
    if (callback) callback->OnSuccess();
}

void V2TIMManagerImpl::DismissGroup(const V2TIMString& groupID, V2TIMCallback* callback) {
    // Simulate dismissing the group
    if (callback) callback->OnSuccess();
}

// User Info
void V2TIMManagerImpl::GetUsersInfo(const V2TIMStringVector& userIDList, V2TIMValueCallback<V2TIMUserFullInfoVector>* callback) {
    V2TIMUserFullInfoVector infos; // Corrected typo: TXV2TIMUserFullInfoVector -> V2TIMUserFullInfoVector
    Tox* tox = ToxManager::getInstance().getTox();
    if (!tox) {
        if (callback) callback->OnError(ERR_SDK_NOT_INITIALIZED, "Tox not initialized");
        return;
    }

    for (const auto& userID : userIDList) {
        V2TIMUserFullInfo info;
        info.userID = userID;

        // Assume userID is the hex public key string
        uint8_t pub_key[TOX_PUBLIC_KEY_SIZE];
        // if (V2TIMUtils::HexToBytes(userID.CString(), pub_key, TOX_PUBLIC_KEY_SIZE)) {
        if (ToxUtil::tox_hex_to_bytes(userID.CString(), userID.Length(), pub_key, TOX_PUBLIC_KEY_SIZE)) {
            TOX_ERR_FRIEND_BY_PUBLIC_KEY err_find;
            uint32_t friend_number = tox_friend_by_public_key(tox, pub_key, &err_find);
            if (err_find == TOX_ERR_FRIEND_BY_PUBLIC_KEY_OK) {
                // Get Nickname
                TOX_ERR_FRIEND_QUERY err_name;
                size_t name_size = tox_friend_get_name_size(tox, friend_number, &err_name);
                if (err_name == TOX_ERR_FRIEND_QUERY_OK && name_size > 0) {
                    std::vector<uint8_t> name_buffer(name_size);
                    tox_friend_get_name(tox, friend_number, name_buffer.data(), &err_name);
                    if (err_name == TOX_ERR_FRIEND_QUERY_OK) {
                         // Assuming V2TIMString can be constructed from uint8_t* and size
                        info.nickName = V2TIMString(reinterpret_cast<const char*>(name_buffer.data()), name_size);
                    }
                }
                // TODO: Get other fields like faceURL, selfSignature etc. (Tox has limited profile data)
                // info.status = ...; // V2TIMUserFullInfo doesn't have status. Use GetUserStatus instead.
            }
        }
        infos.PushBack(info); // Corrected: push_back -> PushBack (assuming V2TIM vector type)
    }
    if (callback) callback->OnSuccess(infos);
}

// Commented out SetSelfStatus due to cast issue
// void V2TIMManagerImpl::SetSelfStatus(const V2TIMUserStatus& status, V2TIMCallback* callback) {
//     // TODO: Need mapping from V2TIMUserStatus to TOX_USER_STATUS
//     // tox_self_set_status(ToxManager::getInstance().getTox(), (TOX_USER_STATUS)status);
//     if (callback) callback->OnSuccess();
// }

void V2TIMManagerImpl::SubscribeUserInfo(const V2TIMStringVector& userIDList, V2TIMCallback* callback) {
    // Simulate user info subscription
    if (callback) callback->OnSuccess();
}

void V2TIMManagerImpl::UnsubscribeUserInfo(const V2TIMStringVector& userIDList, V2TIMCallback* callback) {
    // Simulate user info unsubscription
    if (callback) callback->OnSuccess();
}

void V2TIMManagerImpl::SetSelfInfo(const V2TIMUserFullInfo& info, V2TIMCallback* callback) {
    Tox* tox = ToxManager::getInstance().getTox();
    if (!tox) {
        if (callback) callback->OnError(ERR_SDK_NOT_INITIALIZED, "Tox not initialized");
        return;
    }

    // Set Nickname
    TOX_ERR_SET_INFO err_name;
    tox_self_set_name(tox, reinterpret_cast<const uint8_t*>(info.nickName.CString()), info.nickName.Length(), &err_name);
    if (err_name != TOX_ERR_SET_INFO_OK) {
        if (callback) callback->OnError(ERR_INVALID_PARAMETERS, "Set self nickname failed");
        return; // Return on first error for simplicity
    }

    // Set Status Message (if mapped to V2TIMUserFullInfo)
    // Assuming selfSignature maps to status message
    TOX_ERR_SET_INFO err_status;
    tox_self_set_status_message(tox, reinterpret_cast<const uint8_t*>(info.selfSignature.CString()), info.selfSignature.Length(), &err_status);
    if (err_status != TOX_ERR_SET_INFO_OK) {
         if (callback) callback->OnError(ERR_INVALID_PARAMETERS, "Set self status message failed");
         return;
    }

    // TODO: Map other fields if possible (faceURL, role, level, etc.)

    if (callback) callback->OnSuccess();
}

// Search & Status
void V2TIMManagerImpl::SearchUsers(const V2TIMUserSearchParam& param, V2TIMValueCallback<V2TIMUserSearchResult>* callback) {
    // Simulate search logic
    if (callback) {
        V2TIMUserSearchResult result;
        callback->OnSuccess(result);
    }
}

void V2TIMManagerImpl::GetUserStatus(const V2TIMStringVector& userIDList, V2TIMValueCallback<V2TIMUserStatusVector>* callback) {
    // 模拟查询逻辑
    if (callback) {
        V2TIMUserStatusVector statusList;
        callback->OnSuccess(statusList);
    }
}

void V2TIMManagerImpl::SetSelfStatus(const V2TIMUserStatus& status, V2TIMCallback* callback) {
    // 模拟设置状态逻辑
    if (callback) {
        callback->OnSuccess();
    }
}

void V2TIMManagerImpl::SubscribeUserStatus(const V2TIMStringVector& userIDList, V2TIMCallback* callback) {
    // 模拟订阅逻辑
    if (callback) {
        callback->OnSuccess();
    }
}

void V2TIMManagerImpl::UnsubscribeUserStatus(const V2TIMStringVector& userIDList, V2TIMCallback* callback) {
    // 模拟取消订阅逻辑
    if (callback) {
        callback->OnSuccess();
    }
}

// Advanced Managers
V2TIMMessageManager* V2TIMManagerImpl::GetMessageManager() {
    return V2TIMMessageManagerImpl::GetInstance();
}

V2TIMGroupManager* V2TIMManagerImpl::GetGroupManager() {
    return V2TIMGroupManagerImpl::GetInstance();
}

V2TIMCommunityManager* V2TIMManagerImpl::GetCommunityManager() {
    // V2TIMCommunityManagerImpl is not implemented yet
    return nullptr; 
}

V2TIMConversationManager* V2TIMManagerImpl::GetConversationManager() {
    // V2TIMConversationManagerImpl is not implemented yet
    return nullptr;
}

V2TIMFriendshipManager* V2TIMManagerImpl::GetFriendshipManager() {
    return V2TIMFriendshipManagerImpl::GetInstance(); // Use the Impl singleton
}

V2TIMOfflinePushManager* V2TIMManagerImpl::GetOfflinePushManager() {
    return nullptr; // Placeholder
}

V2TIMSignalingManager* V2TIMManagerImpl::GetSignalingManager() {
    // V2TIMSignalingManagerImpl is not implemented yet
    return nullptr;
}

// Experimental API
void V2TIMManagerImpl::CallExperimentalAPI(const V2TIMString& api, const void* param, V2TIMValueCallback<V2TIMBaseObject>* callback) {
    // 模拟实验性 API 逻辑
    if (callback) {
        callback->OnSuccess(V2TIMBaseObject());
    }
}

// Remove find_conference_by_id helper for now, will be handled within JoinGroup/Map logic
// uint32_t V2TIMManagerImpl::find_conference_by_id(const V2TIMString& groupID) { ... }

// --- Implementation of Internal Handlers ---

void V2TIMManagerImpl::HandleGroupMessage(uint32_t conference_number, uint32_t peer_number, TOX_MESSAGE_TYPE type, const uint8_t* message_data, size_t length) {
    Tox* tox = ToxManager::getInstance().getTox();
    V2TIMMessageManagerImpl* msgManager = V2TIMMessageManagerImpl::GetInstance();
    if (!tox || !msgManager || !running_) {
        V2TIM_LOG(kError, "HandleGroupMessage skipped: Dependencies missing or shutting down.");
        return; // Not initialized or shutting down
    }

    V2TIMString groupID;
    V2TIMString senderUserID;

    // Avoid self-messages by comparing public keys
    uint8_t self_pubkey[TOX_PUBLIC_KEY_SIZE];
    tox_self_get_public_key(tox, self_pubkey);
    
    uint8_t sender_pubkey[TOX_PUBLIC_KEY_SIZE];
    TOX_ERR_CONFERENCE_PEER_QUERY err_peer;
    bool got_key = tox_conference_peer_get_public_key(tox, conference_number, peer_number, sender_pubkey, &err_peer);
    
    if (got_key && err_peer == TOX_ERR_CONFERENCE_PEER_QUERY_OK && 
        memcmp(self_pubkey, sender_pubkey, TOX_PUBLIC_KEY_SIZE) == 0) {
        V2TIM_LOG(kInfo, "HandleGroupMessage: Ignoring self-message in group.");
        return; // Don't process messages sent by self
    }

    // --- Find GroupID --- 
    {
        std::lock_guard<std::mutex> lock(mutex_); // Protect map access
        auto it = conference_number_to_group_id_.find(conference_number);
        if (it == conference_number_to_group_id_.end()) {
            V2TIM_LOG(kError, "Received message for unknown conference number {}", conference_number);
            return;
        }
        groupID = it->second;
    }

    // --- Find Sender UserID (Public Key Hex) --- 
    char sender_hex_id[TOX_PUBLIC_KEY_SIZE * 2 + 1];
    std::string hex_id = ToxUtil::tox_bytes_to_hex(sender_pubkey, TOX_PUBLIC_KEY_SIZE);
    strcpy(sender_hex_id, hex_id.c_str());
    sender_hex_id[TOX_PUBLIC_KEY_SIZE * 2] = '\0'; 
    senderUserID = sender_hex_id;

    V2TIM_LOG(kInfo, "Received group msg type {} in group {} from {}", type, groupID.CString(), senderUserID.CString());

    // --- Create V2TIMMessage Object --- 
    V2TIMMessage v2_message; // Will be populated based on type
    bool message_created = false;

    if (type == TOX_MESSAGE_TYPE_NORMAL) {
        V2TIMString messageText(reinterpret_cast<const char*>(message_data), length);
        v2_message = msgManager->CreateTextMessage(messageText);
        message_created = true;
    } else if (type == TOX_MESSAGE_TYPE_ACTION) {
        V2TIMBuffer customData(message_data, length);
        v2_message = msgManager->CreateCustomMessage(customData);
        message_created = true;
    } else {
        V2TIM_LOG(kWarning, "Received unhandled group message type {}", type);
        return; // Don't notify for unsupported types
    }

    if (message_created) {
        // --- Populate received message fields --- 
        v2_message.sender = senderUserID;
        v2_message.groupID = groupID;
        v2_message.isSelf = false; // Message received from others
        v2_message.status = V2TIM_MSG_STATUS_SEND_SUCC; // Mark as received successfully
        // Use current time as Tox doesn't provide a timestamp for received messages
        v2_message.timestamp = std::chrono::duration_cast<std::chrono::seconds>(
                                    std::chrono::system_clock::now().time_since_epoch()).count();
        // TODO: Potentially retrieve sender Nickname/FaceURL here if needed/cached
        // v2_message.nickName = ... 
        // v2_message.faceURL = ...
        
        // --- Notify Advanced Listeners --- 
        msgManager->NotifyAdvancedListenersReceivedMessage(v2_message);

        // --- Notify Simple Listeners (Optional - Keep for now) --- 
        std::vector<V2TIMSimpleMsgListener*> listeners_to_notify;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            listeners_to_notify.assign(simple_msg_listeners_.begin(), simple_msg_listeners_.end());
        }
        
        // Check message type from the message's elemList first element
        if (!v2_message.elemList.Empty()) {
            V2TIMElem* elem = v2_message.elemList[0];
            if (elem->elemType == V2TIM_ELEM_TYPE_TEXT) {
                V2TIMTextElem* textElem = static_cast<V2TIMTextElem*>(elem);
                
                // Create simplified member info for the notification
                V2TIMGroupMemberFullInfo senderInfo;
                senderInfo.userID = senderUserID;
                // TODO: Add more sender info if available
                
                for (V2TIMSimpleMsgListener* listener : listeners_to_notify) {
                    if (listener) listener->OnRecvGroupTextMessage(v2_message.msgID, groupID, senderInfo, textElem->text);
                }
            } else if (elem->elemType == V2TIM_ELEM_TYPE_CUSTOM) {
                V2TIMCustomElem* customElem = static_cast<V2TIMCustomElem*>(elem);
                
                // Create simplified member info for the notification
                V2TIMGroupMemberFullInfo senderInfo;
                senderInfo.userID = senderUserID;
                // TODO: Add more sender info if available
                
                for (V2TIMSimpleMsgListener* listener : listeners_to_notify) {
                    if (listener) listener->OnRecvGroupCustomMessage(v2_message.msgID, groupID, senderInfo, customElem->data);
                }
            }
        }
    }
}

void V2TIMManagerImpl::HandleFriendMessage(uint32_t friend_number, TOX_MESSAGE_TYPE type, const uint8_t* message_data, size_t length) {
    Tox* tox = ToxManager::getInstance().getTox();
    V2TIMMessageManagerImpl* msgManager = V2TIMMessageManagerImpl::GetInstance();
    if (!tox || !msgManager || !running_) {
        V2TIM_LOG(kError, "HandleFriendMessage skipped: Dependencies missing or shutting down.");
        return; // Not initialized or shutting down
    }

    V2TIMString senderUserID;

    // --- Find Sender UserID (Public Key Hex) --- 
    uint8_t sender_pubkey[TOX_PUBLIC_KEY_SIZE];
    TOX_ERR_FRIEND_GET_PUBLIC_KEY err_key;
    bool got_key = tox_friend_get_public_key(tox, friend_number, sender_pubkey, &err_key);
    if (!got_key || err_key != TOX_ERR_FRIEND_GET_PUBLIC_KEY_OK) {
        V2TIM_LOG(kError, "Failed to get public key for friend number {}", friend_number);
        return;
    }
    char sender_hex_id[TOX_PUBLIC_KEY_SIZE * 2 + 1];
    std::string hex_id = ToxUtil::tox_bytes_to_hex(sender_pubkey, TOX_PUBLIC_KEY_SIZE);
    strcpy(sender_hex_id, hex_id.c_str());
    sender_hex_id[TOX_PUBLIC_KEY_SIZE * 2] = '\0';
    senderUserID = sender_hex_id;

    V2TIM_LOG(kInfo, "Received C2C msg type {} from {} (friend {})", type, senderUserID.CString(), friend_number);

    // --- Create V2TIMMessage Object --- 
    V2TIMMessage v2_message; // Will be populated based on type
    bool message_created = false;

    if (type == TOX_MESSAGE_TYPE_NORMAL) {
        V2TIMString messageText(reinterpret_cast<const char*>(message_data), length);
        v2_message = msgManager->CreateTextMessage(messageText);
        message_created = true;
    } else if (type == TOX_MESSAGE_TYPE_ACTION) {
        V2TIMBuffer customData(message_data, length);
        v2_message = msgManager->CreateCustomMessage(customData);
        message_created = true;
    } else {
        V2TIM_LOG(kWarning, "Received unhandled C2C message type {}", type);
        return; // Don't notify for unsupported types
    }

    if (message_created) {
         // --- Populate received message fields --- 
        v2_message.sender = senderUserID;
        v2_message.userID = senderUserID; // For C2C, userID is the sender
        v2_message.groupID = ""; // Clear groupID for C2C
        v2_message.isSelf = false; 
        v2_message.status = V2TIM_MSG_STATUS_SEND_SUCC;
        v2_message.timestamp = std::chrono::duration_cast<std::chrono::seconds>(
                                    std::chrono::system_clock::now().time_since_epoch()).count();
        // TODO: Potentially retrieve sender Nickname/FaceURL here if needed/cached

        // --- Notify Advanced Listeners --- 
        msgManager->NotifyAdvancedListenersReceivedMessage(v2_message);

        // --- Notify Simple Listeners (Optional - Keep for now) --- 
        std::vector<V2TIMSimpleMsgListener*> listeners_to_notify;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            listeners_to_notify.assign(simple_msg_listeners_.begin(), simple_msg_listeners_.end());
        }
        
        // Check message type from the message's elemList first element
        if (!v2_message.elemList.Empty()) {
            V2TIMElem* elem = v2_message.elemList[0];
            if (elem->elemType == V2TIM_ELEM_TYPE_TEXT) {
                V2TIMTextElem* textElem = static_cast<V2TIMTextElem*>(elem);
                
                // Create simplified user info for the notification
                V2TIMUserFullInfo senderInfo;
                senderInfo.userID = senderUserID;
                // TODO: Add more sender info if available
                
                for (V2TIMSimpleMsgListener* listener : listeners_to_notify) {
                    if (listener) listener->OnRecvC2CTextMessage(v2_message.msgID, senderInfo, textElem->text);
                }
            } else if (elem->elemType == V2TIM_ELEM_TYPE_CUSTOM) {
                V2TIMCustomElem* customElem = static_cast<V2TIMCustomElem*>(elem);
                
                // Create simplified user info for the notification
                V2TIMUserFullInfo senderInfo;
                senderInfo.userID = senderUserID;
                // TODO: Add more sender info if available
                
                for (V2TIMSimpleMsgListener* listener : listeners_to_notify) {
                    if (listener) listener->OnRecvC2CCustomMessage(v2_message.msgID, senderInfo, customElem->data);
                }
            }
        }
    }
}

void V2TIMManagerImpl::HandleSelfConnectionStatus(TOX_CONNECTION connection_status) {
    V2TIM_LOG(kInfo, "HandleSelfConnectionStatus: {}", connection_status);
    std::vector<V2TIMSDKListener*> listeners_to_notify;
    {
        std::lock_guard<std::mutex> lock(mutex_); // Protect listener access
        listeners_to_notify.assign(sdk_listeners_.begin(), sdk_listeners_.end());
    }

    for (V2TIMSDKListener* listener : listeners_to_notify) {
        if (!listener) continue;
        
        switch (connection_status) {
            case TOX_CONNECTION_NONE:
                // Could be initial state or disconnected state
                // Let's assume it means disconnected for now, might need refinement
                listener->OnConnectFailed(ERR_SDK_NET_DISCONNECT, "Disconnected from Tox network");
                break;
            case TOX_CONNECTION_TCP:
            case TOX_CONNECTION_UDP:
                // Consider both as connected
                // Should we differentiate between first connection and reconnection?
                // V2TIM has OnConnecting -> OnConnectSuccess
                // Maybe call OnConnecting on first non-NONE state?
                // For simplicity, call OnConnectSuccess directly for now.
                listener->OnConnectSuccess();
                break;
        }
        // TODO: Map other V2TIMSDKListener connection callbacks like OnConnecting, OnKickedOffline?
        // OnKickedOffline might need a specific signal/callback from Tox, not just connection status.
    }
}

void V2TIMManagerImpl::HandleFriendRequest(const uint8_t* public_key, const uint8_t* message_data, size_t length) {
    char sender_hex_id[TOX_PUBLIC_KEY_SIZE * 2 + 1];
    std::string hex_id = ToxUtil::tox_bytes_to_hex(public_key, TOX_PUBLIC_KEY_SIZE);
    strcpy(sender_hex_id, hex_id.c_str());
    sender_hex_id[TOX_PUBLIC_KEY_SIZE * 2] = '\0';
    V2TIMString senderUserID = sender_hex_id;
    V2TIMString requestMessage(reinterpret_cast<const char*>(message_data), length);

    V2TIM_LOG(kInfo, "HandleFriendRequest from {} with message: {}", senderUserID.CString(), requestMessage.CString());

    V2TIMFriendshipManagerImpl* fm = V2TIMFriendshipManagerImpl::GetInstance();
    if (fm) { 
        V2TIMFriendApplication application;
        application.userID = senderUserID;
        application.addWording = requestMessage;
        application.addSource = "Tox"; // Default source
        application.type = V2TIM_FRIEND_APPLICATION_COME_IN; 
        // TODO: Get nickname/faceURL for the application if possible via GetUsersInfo?
        // Requires potentially making GetUsersInfo synchronous or caching.
        
        V2TIMFriendApplicationVector applications;
        applications.PushBack(application);
        fm->NotifyFriendApplicationListAdded(applications); 
    } else { 
        V2TIM_LOG(kWarning, "Cannot notify FriendshipListener: FriendshipManagerImpl instance is null.");
    }
}

void V2TIMManagerImpl::HandleFriendName(uint32_t friend_number, const uint8_t* name_data, size_t length) {
    Tox* tox = ToxManager::getInstance().getTox();
    if (!tox || !running_) return;
    V2TIMFriendshipManagerImpl* fm = V2TIMFriendshipManagerImpl::GetInstance();
    if (!fm) return;

    V2TIMString friendUserID;
    V2TIMString friendName(reinterpret_cast<const char*>(name_data), length);
    uint8_t pubkey[TOX_PUBLIC_KEY_SIZE];
    TOX_ERR_FRIEND_GET_PUBLIC_KEY err_key;
    if (tox_friend_get_public_key(tox, friend_number, pubkey, &err_key) && err_key == TOX_ERR_FRIEND_GET_PUBLIC_KEY_OK) {
        char hex_id[TOX_PUBLIC_KEY_SIZE * 2 + 1];
        std::string hex_id_str = ToxUtil::tox_bytes_to_hex(pubkey, TOX_PUBLIC_KEY_SIZE);
        strcpy(hex_id, hex_id_str.c_str());
        hex_id[TOX_PUBLIC_KEY_SIZE * 2] = '\0';
        friendUserID = hex_id;

        V2TIM_LOG(kInfo, "HandleFriendName: Friend {} ({}) changed name to: {}", friendUserID.CString(), friend_number, friendName.CString());
        
        V2TIMFriendInfoResult infoResult;
        infoResult.resultCode = 0;
        infoResult.friendInfo.userID = friendUserID;
        infoResult.friendInfo.userFullInfo.nickName = friendName;
        
        V2TIMFriendInfoResultVector infoResultVector;
        infoResultVector.PushBack(infoResult);
        // Only include nickname in this change notification
        fm->NotifyFriendInfoChanged(infoResultVector);

    } else {
        V2TIM_LOG(kError, "HandleFriendName: Failed to get public key for friend number {}", friend_number);
    }
}

void V2TIMManagerImpl::HandleFriendStatusMessage(uint32_t friend_number, const uint8_t* message_data, size_t length) {
     Tox* tox = ToxManager::getInstance().getTox();
    if (!tox || !running_) return;
    V2TIMFriendshipManagerImpl* fm = V2TIMFriendshipManagerImpl::GetInstance();
    if (!fm) return;

    V2TIMString friendUserID;
    V2TIMString statusMessage(reinterpret_cast<const char*>(message_data), length);
    uint8_t pubkey[TOX_PUBLIC_KEY_SIZE];
    TOX_ERR_FRIEND_GET_PUBLIC_KEY err_key;
    if (tox_friend_get_public_key(tox, friend_number, pubkey, &err_key) && err_key == TOX_ERR_FRIEND_GET_PUBLIC_KEY_OK) {
        char hex_id[TOX_PUBLIC_KEY_SIZE * 2 + 1];
        std::string hex_id_str = ToxUtil::tox_bytes_to_hex(pubkey, TOX_PUBLIC_KEY_SIZE);
        strcpy(hex_id, hex_id_str.c_str());
        hex_id[TOX_PUBLIC_KEY_SIZE * 2] = '\0';
        friendUserID = hex_id;

        V2TIM_LOG(kInfo, "HandleFriendStatusMessage: Friend {} ({}) changed status message to: {}", friendUserID.CString(), friend_number, statusMessage.CString());

        V2TIMFriendInfoResult infoResult;
        infoResult.resultCode = 0;
        infoResult.friendInfo.userID = friendUserID;
        infoResult.friendInfo.userFullInfo.selfSignature = statusMessage; // Map Tox status message to V2TIM selfSignature
        
        V2TIMFriendInfoResultVector infoResultVector;
        infoResultVector.PushBack(infoResult);
        // Only include status message in this change notification
        fm->NotifyFriendInfoChanged(infoResultVector);
    } else {
        V2TIM_LOG(kError, "HandleFriendStatusMessage: Failed to get public key for friend number {}", friend_number);
    }
}

void V2TIMManagerImpl::HandleFriendStatus(uint32_t friend_number, TOX_USER_STATUS status) {
    Tox* tox = ToxManager::getInstance().getTox();
    if (!tox || !running_) return;

    V2TIMString friendUserID;

    // Get UserID from friend_number
    uint8_t pubkey[TOX_PUBLIC_KEY_SIZE];
    TOX_ERR_FRIEND_GET_PUBLIC_KEY err_key;
    if (tox_friend_get_public_key(tox, friend_number, pubkey, &err_key) && err_key == TOX_ERR_FRIEND_GET_PUBLIC_KEY_OK) {
        char hex_id[TOX_PUBLIC_KEY_SIZE * 2 + 1];
        std::string hex_id_str = ToxUtil::tox_bytes_to_hex(pubkey, TOX_PUBLIC_KEY_SIZE);
        strcpy(hex_id, hex_id_str.c_str());
        hex_id[TOX_PUBLIC_KEY_SIZE * 2] = '\0';
        friendUserID = hex_id;
    } else {
        V2TIM_LOG(kError, "HandleFriendStatus: Failed to get public key for friend number {}", friend_number);
        return;
    }

    // Map TOX_USER_STATUS to V2TIMUserStatusType
    V2TIMUserStatus v2_status;
    v2_status.userID = friendUserID;
    switch (status) {
        case TOX_USER_STATUS_NONE:
            v2_status.statusType = V2TIM_USER_STATUS_OFFLINE;
            break;
        case TOX_USER_STATUS_AWAY:
            v2_status.statusType = V2TIM_USER_STATUS_OFFLINE; // Or map to a custom status? V2TIM only has Online/Offline/Unkown.
            break;
        case TOX_USER_STATUS_BUSY:
             v2_status.statusType = V2TIM_USER_STATUS_ONLINE; // Map Busy to Online for simplicity?
            break;
        default: // Includes TOX_USER_STATUS_INVALID and any others
             v2_status.statusType = V2TIM_USER_STATUS_UNKNOWN;
             break;
    }
     // v2_status.customStatus = ...; // Tox doesn't provide custom status string in this callback

    V2TIM_LOG(kInfo, "HandleFriendStatus: Friend {} ({}) changed status to: {} (V2TIM type: {})", friendUserID.CString(), friend_number, status, v2_status.statusType);

    // Notify SDK Listeners
    std::vector<V2TIMSDKListener*> listeners_to_notify;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        listeners_to_notify.assign(sdk_listeners_.begin(), sdk_listeners_.end());
    }
    
    V2TIMUserStatusVector statusVector;
    statusVector.PushBack(v2_status);
    
    for (V2TIMSDKListener* listener : listeners_to_notify) {
        if (listener) {
            listener->OnUserStatusChanged(statusVector); // Notify with the status vector
        }
    }
}

void V2TIMManagerImpl::HandleGroupTitle(uint32_t conference_number, uint32_t peer_number, const uint8_t* title_data, size_t length) {
    Tox* tox = ToxManager::getInstance().getTox();
    if (!tox || !running_) return;

    V2TIMString groupID;
    V2TIMString groupName(reinterpret_cast<const char*>(title_data), length);
    V2TIMString opUserID;

    // Find GroupID
    {
        std::lock_guard<std::mutex> lock(mutex_); 
        auto it = conference_number_to_group_id_.find(conference_number);
        if (it == conference_number_to_group_id_.end()) return; // Unknown group
        groupID = it->second;
    }

    // Find Operator UserID (who changed the title)
    uint8_t op_pubkey[TOX_PUBLIC_KEY_SIZE];
    TOX_ERR_CONFERENCE_PEER_QUERY err_peer;
    if (tox_conference_peer_get_public_key(tox, conference_number, peer_number, op_pubkey, &err_peer) && err_peer == TOX_ERR_CONFERENCE_PEER_QUERY_OK) {
         char hex_id[TOX_PUBLIC_KEY_SIZE * 2 + 1];
        std::string hex_id_str = ToxUtil::tox_bytes_to_hex(op_pubkey, TOX_PUBLIC_KEY_SIZE);
        strcpy(hex_id, hex_id_str.c_str());
        hex_id[TOX_PUBLIC_KEY_SIZE * 2] = '\0'; 
        opUserID = hex_id;
    } else {
        V2TIM_LOG(kWarning, "HandleGroupTitle: Could not get operator user ID for peer {} in group {}", peer_number, groupID.CString());
        // Continue, but opUserID will be empty
    }
    
    V2TIM_LOG(kInfo, "HandleGroupTitle: Group {} title changed to '{}' by {} ({})", groupID.CString(), groupName.CString(), opUserID.CString(), peer_number);

    // Notify Listeners
    std::vector<V2TIMGroupListener*> listeners_copy;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        listeners_copy.assign(group_listeners_.begin(), group_listeners_.end());
    }

    // Construct V2TIMGroupChangeInfo for the name change
    V2TIMGroupChangeInfo changeInfo;
    changeInfo.type = V2TIM_GROUP_INFO_CHANGE_TYPE_NAME;
    changeInfo.value = groupName;
    
    // Create a vector for group changes
    V2TIMGroupChangeInfoVector changeInfoVector;
    changeInfoVector.PushBack(changeInfo);

    for (V2TIMGroupListener* listener : listeners_copy) {
        if (listener) {
            // Construct V2TIMGroupMemberInfo for the operator
            V2TIMGroupMemberInfo opMemberInfo;
            opMemberInfo.userID = opUserID;
            // TODO: Fill other opMemberInfo fields if possible (nickname, role etc.)
            
            // OnGroupInfoChanged only takes two parameters
            listener->OnGroupInfoChanged(groupID, changeInfoVector);
            
            // Note: If you need to pass the operator info, consider adding it to your own field
            // or using a different callback that accepts the operator parameter
        }
    }
}

void V2TIMManagerImpl::HandleGroupPeerName(uint32_t conference_number, uint32_t peer_number, const uint8_t* name_data, size_t length) {
    Tox* tox = ToxManager::getInstance().getTox();
    if (!tox || !running_) return;

    V2TIMString groupID;
    V2TIMString memberUserID;
    V2TIMString memberName(reinterpret_cast<const char*>(name_data), length);

    // Find GroupID
    {
        std::lock_guard<std::mutex> lock(mutex_); 
        auto it = conference_number_to_group_id_.find(conference_number);
        if (it == conference_number_to_group_id_.end()) return; // Unknown group
        groupID = it->second;
    }

    // Find Member UserID 
    uint8_t member_pubkey[TOX_PUBLIC_KEY_SIZE];
    TOX_ERR_CONFERENCE_PEER_QUERY err_peer;
    if (tox_conference_peer_get_public_key(tox, conference_number, peer_number, member_pubkey, &err_peer) && err_peer == TOX_ERR_CONFERENCE_PEER_QUERY_OK) {
         char hex_id[TOX_PUBLIC_KEY_SIZE * 2 + 1];
        std::string hex_id_str = ToxUtil::tox_bytes_to_hex(member_pubkey, TOX_PUBLIC_KEY_SIZE);
        strcpy(hex_id, hex_id_str.c_str());
        hex_id[TOX_PUBLIC_KEY_SIZE * 2] = '\0'; 
        memberUserID = hex_id;
    } else {
        V2TIM_LOG(kError, "HandleGroupPeerName: Could not get user ID for peer {} in group {}", peer_number, groupID.CString());
        return;
    }

    V2TIM_LOG(kInfo, "HandleGroupPeerName: Member {} in group {} changed name to '{}'", memberUserID.CString(), groupID.CString(), memberName.CString());

    // Notify Listeners
    std::vector<V2TIMGroupListener*> listeners_copy;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        listeners_copy.assign(group_listeners_.begin(), group_listeners_.end());
    }

    V2TIMGroupMemberChangeInfo changeInfo;
    changeInfo.userID = memberUserID;
    changeInfo.muteTime = 0; // Name change doesn't involve mute time
    
    V2TIMGroupMemberChangeInfoVector changeInfoVector;
    changeInfoVector.PushBack(changeInfo);

    for (V2TIMGroupListener* listener : listeners_copy) {
        if (listener) {
             // V2TIM lacks a specific "member name changed" callback. 
             // Use OnMemberInfoChanged, but it requires a list of changes.
             // For simplicity, we send just the name change.
             V2TIM_LOG(kWarning, "Mapping PeerName change to OnMemberInfoChanged (may lack other details).");
            listener->OnMemberInfoChanged(groupID, changeInfoVector);
            // Ideally, fetch full member info and send that?
        }
    }
}

void V2TIMManagerImpl::HandleGroupPeerListChanged(uint32_t conference_number) {
     Tox* tox = ToxManager::getInstance().getTox();
    if (!tox || !running_) return;
    V2TIM_LOG(kInfo, "HandleGroupPeerListChanged called for conference {}", conference_number);
    // This callback is vague. It means *something* changed in the peer list (join, leave, maybe name/status change trigger it too?).
    // Tox doesn't tell us *who* joined or left here.
    // A robust implementation would:
    // 1. Get the current list of peers in the conference using tox_conference_get_peer_public_keys or similar.
    // 2. Compare it to a previously stored list for this conference.
    // 3. Determine who joined and who left.
    // 4. Call OnMemberEnter for new peers and OnMemberLeave for departed peers.
    
    // For now, just log it.
    V2TIM_LOG(kWarning, "HandleGroupPeerListChanged: Detailed join/leave detection not implemented.");

    // Potential simple notification (less accurate):
    // V2TIMString groupID;
    // { ... find groupID ... }
    // std::vector<V2TIMGroupListener*> listeners_copy;
    // { ... get listeners ... }
    // V2TIMGroupMemberInfo member; // Need a way to know WHO joined/left
    // for (V2TIMGroupListener* listener : listeners_copy) {
    //     if (listener) {
             // listener->OnMemberEnter(groupID, {member}); 
             // listener->OnMemberLeave(groupID, member);
    //     }
    // }
}

// Helper method to access private data
bool V2TIMManagerImpl::GetGroupNumberFromID(const V2TIMString& groupID, uint32_t& group_number) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = group_id_to_conference_number_.find(groupID);
    if (it == group_id_to_conference_number_.end()) {
        return false;
    }
    group_number = it->second;
    return true;
}