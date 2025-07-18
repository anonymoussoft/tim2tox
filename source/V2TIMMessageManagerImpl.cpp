#include "V2TIMMessageManagerImpl.h"
#include "V2TIMManagerImpl.h" // To call Send C2C/Group messages
#include "ToxManager.h" // Potentially needed for direct Tox calls?
#include <V2TIMErrorCode.h>
#include <chrono> // For timestamps
#include <cstdlib> // For rand()
#include "TIMResultDefine.h"

// Initialize singleton instance
V2TIMMessageManagerImpl* V2TIMMessageManagerImpl::GetInstance() {
    static V2TIMMessageManagerImpl instance;
    return &instance;
}

// Constructor
V2TIMMessageManagerImpl::V2TIMMessageManagerImpl() 
    : db_(":memory:", SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE) // 初始化一个内存数据库
{
    // TODO: Initialize database connection if needed
    V2TIM_LOG(kInfo, "V2TIMMessageManagerImpl initialized.");
}

// Destructor
V2TIMMessageManagerImpl::~V2TIMMessageManagerImpl() {
    // TODO: Close database connection if needed
    V2TIM_LOG(kInfo, "V2TIMMessageManagerImpl destroyed.");
}

// --- Listener Management ---
void V2TIMMessageManagerImpl::AddAdvancedMsgListener(V2TIMAdvancedMsgListener* listener) {
    std::lock_guard<std::mutex> lock(listener_mutex_);
    if (listener) {
        listeners_.insert(listener);
    }
}

void V2TIMMessageManagerImpl::RemoveAdvancedMsgListener(V2TIMAdvancedMsgListener* listener) {
    std::lock_guard<std::mutex> lock(listener_mutex_);
    if (listener) {
        listeners_.erase(listener);
    }
}

// --- Internal Helper for Creating Base Message ---
V2TIMMessage V2TIMMessageManagerImpl::CreateBaseMessage() {
    V2TIMMessage msg;
    // Generate a unique message ID (client-side)
    uint64_t timestamp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                            std::chrono::system_clock::now().time_since_epoch())
                            .count();
    uint32_t random_part = std::rand();
    char msg_id_buffer[64];
    // Note: ID format might differ from V2TIMManager simple sends
    snprintf(msg_id_buffer, sizeof(msg_id_buffer), "m%llu-%u", timestamp_ms, random_part); 
    msg.msgID = msg_id_buffer;

    msg.timestamp = timestamp_ms / 1000; // V2TIM timestamp is in seconds
    msg.sender = V2TIMManagerImpl::GetInstance()->GetLoginUser(); // Set current user as sender
    msg.status = V2TIM_MSG_STATUS_SENDING;
    msg.isSelf = true;
    // Other fields (receiver, groupID, etc.) set by SendMessage
    return msg;
}

// --- Message Creation Methods (Initial Implementations) ---
V2TIMMessage V2TIMMessageManagerImpl::CreateTextMessage(const V2TIMString& text) {
    V2TIMMessage msg = CreateBaseMessage();
    V2TIMTextElem* textElem = new V2TIMTextElem();
    textElem->elemType = V2TIM_ELEM_TYPE_TEXT;
    textElem->text = text;
    msg.elemList.PushBack(textElem);
    V2TIM_LOG(kInfo, "Created Text Message: %s", msg.msgID.CString());
    return msg;
}

// --- Helper for Reporting Not Implemented --- 
void V2TIMMessageManagerImpl::ReportNotImplemented(V2TIMCallback* callback) {
    if (callback) {
        callback->OnError(ERR_SDK_NOT_SUPPORTED, "Feature not implemented in tim2tox");
    }
}

template <typename T>
void V2TIMMessageManagerImpl::ReportNotImplemented(V2TIMValueCallback<T>* callback) {
    if (callback) {
        callback->OnError(ERR_SDK_NOT_SUPPORTED, "Feature not implemented in tim2tox");
    }
}

template <typename T>
void V2TIMMessageManagerImpl::ReportNotImplemented(V2TIMCompleteCallback<T>* callback) {
    if (callback) {
        callback->OnComplete(ERR_SDK_NOT_SUPPORTED, "Feature not implemented in tim2tox", T());
    }
}

// --- Placeholder Implementations for Remaining Methods ---

V2TIMMessage V2TIMMessageManagerImpl::CreateTextAtMessage(const V2TIMString& text, const V2TIMStringVector& atUserList) {
    // Basic implementation: create text message, add atUserList to V2TIMMessage
    V2TIMMessage msg = CreateTextMessage(text);
    msg.groupAtUserList = atUserList;
    // TODO: Actual sending logic needs to handle @ users potentially
    V2TIM_LOG(kWarning, "CreateTextAtMessage created basic text message, @ handling not fully implemented for send.");
    return msg;
}

V2TIMMessage V2TIMMessageManagerImpl::CreateCustomMessage(const V2TIMBuffer& data) {
    V2TIMMessage msg = CreateBaseMessage();
    V2TIMCustomElem* customElem = new V2TIMCustomElem();
    customElem->elemType = V2TIM_ELEM_TYPE_CUSTOM;
    customElem->data = data;
    msg.elemList.PushBack(customElem);
    V2TIM_LOG(kInfo, "Created Custom Message: %s", msg.msgID.CString());
    return msg;
}

V2TIMMessage V2TIMMessageManagerImpl::CreateCustomMessage(const V2TIMBuffer& data, const V2TIMString& description, const V2TIMString& extension) {
    V2TIMMessage msg = CreateCustomMessage(data);
    
    // Get the custom element from the elemList and update its properties
    if (msg.elemList.Size() > 0) {
        V2TIMElem* elem = msg.elemList[0];
        if (elem && elem->elemType == V2TIM_ELEM_TYPE_CUSTOM) {
            V2TIMCustomElem* customElem = static_cast<V2TIMCustomElem*>(elem);
            customElem->desc = description;
            customElem->extension = extension;
        }
    }
    
    return msg;
}

V2TIMMessage V2TIMMessageManagerImpl::CreateImageMessage(const V2TIMString& imagePath) {
    V2TIM_LOG(kError, "CreateImageMessage not implemented.");
    // TODO: Read image, potentially upload, create V2TIMImageElem
    return CreateBaseMessage(); // Return empty message
}

V2TIMMessage V2TIMMessageManagerImpl::CreateSoundMessage(const V2TIMString& soundPath, uint32_t duration) {
    V2TIM_LOG(kError, "CreateSoundMessage not implemented.");
    // TODO: Read sound file, potentially upload, create V2TIMSoundElem
    return CreateBaseMessage();
}

V2TIMMessage V2TIMMessageManagerImpl::CreateVideoMessage(const V2TIMString& videoFilePath, const V2TIMString& type, uint32_t duration, const V2TIMString& snapshotPath) {
    V2TIM_LOG(kError, "CreateVideoMessage not implemented.");
    // TODO: Read video/snapshot, potentially upload, create V2TIMVideoElem
    return CreateBaseMessage();
}

V2TIMMessage V2TIMMessageManagerImpl::CreateFileMessage(const V2TIMString& filePath, const V2TIMString& fileName) {
     V2TIM_LOG(kError, "CreateFileMessage not implemented.");
    // TODO: Read file, potentially upload, create V2TIMFileElem
    return CreateBaseMessage();
}

V2TIMMessage V2TIMMessageManagerImpl::CreateLocationMessage(const V2TIMString& desc, double longitude, double latitude) {
    V2TIMMessage msg = CreateBaseMessage();
    V2TIMLocationElem* locationElem = new V2TIMLocationElem();
    locationElem->elemType = V2TIM_ELEM_TYPE_LOCATION;
    locationElem->desc = desc;
    locationElem->longitude = longitude;
    locationElem->latitude = latitude;
    msg.elemList.PushBack(locationElem);
    return msg;
}

V2TIMMessage V2TIMMessageManagerImpl::CreateFaceMessage(uint32_t index, const V2TIMBuffer& data) {
    V2TIMMessage msg = CreateBaseMessage();
    V2TIMFaceElem* faceElem = new V2TIMFaceElem();
    faceElem->elemType = V2TIM_ELEM_TYPE_FACE;
    faceElem->index = index;
    faceElem->data = data;
    msg.elemList.PushBack(faceElem);
    return msg;
}

V2TIMMessage V2TIMMessageManagerImpl::CreateMergerMessage(const V2TIMMessageVector& messageList, const V2TIMString& title, const V2TIMStringVector& abstractList, const V2TIMString& compatibleText) {
    V2TIM_LOG(kError, "CreateMergerMessage not implemented.");
    // TODO: Create V2TIMMergerElem
    return CreateBaseMessage();
}

V2TIMMessage V2TIMMessageManagerImpl::CreateForwardMessage(const V2TIMMessage& message) {
     V2TIM_LOG(kError, "CreateForwardMessage not implemented.");
     // TODO: Create new message copying relevant fields from original
    return CreateBaseMessage();
}

V2TIMMessage V2TIMMessageManagerImpl::CreateTargetedGroupMessage(const V2TIMMessage& message, const V2TIMStringVector& receiverList) {
     V2TIM_LOG(kError, "CreateTargetedGroupMessage not implemented.");
     // TODO: Create new message marking targeted receivers
    return CreateBaseMessage();
}

V2TIMMessage V2TIMMessageManagerImpl::CreateAtSignedGroupMessage(const V2TIMMessage& message, const V2TIMStringVector& atUserList) {
     V2TIM_LOG(kWarning, "CreateAtSignedGroupMessage handling not fully implemented.");
     V2TIMMessage msg = message; // Copy original
     msg.groupAtUserList = atUserList;
     return msg;
}

// --- Message Sending --- 
V2TIMString V2TIMMessageManagerImpl::SendMessage(
    V2TIMMessage& message, 
    const V2TIMString& receiver, 
    const V2TIMString& groupID, 
    V2TIMMessagePriority priority, 
    bool onlineUserOnly, 
    const V2TIMOfflinePushInfo& offlinePushInfo, 
    V2TIMSendCallback* callback) 
{
    // --- Validate Destination ---
    bool isC2C = !receiver.Empty();
    bool isGroup = !groupID.Empty();

    if (isC2C && isGroup) {
        V2TIM_LOG(kError, "SendMessage failed: Cannot specify both receiver and groupID.");
        if (callback) callback->OnError(ERR_INVALID_PARAMETERS, "Cannot specify both receiver and groupID");
        message.status = V2TIM_MSG_STATUS_SEND_FAIL; // Mark as failed
        return ""; // Return empty string for error
    }
    if (!isC2C && !isGroup) {
        V2TIM_LOG(kError, "SendMessage failed: Must specify receiver or groupID.");
        if (callback) callback->OnError(ERR_INVALID_PARAMETERS, "Must specify receiver or groupID");
        message.status = V2TIM_MSG_STATUS_SEND_FAIL;
        return "";
    }

    // --- Update Message Fields ---
    message.userID = receiver;  // Store intended receiver
    message.groupID = groupID;  // Store intended group
    message.status = V2TIM_MSG_STATUS_SENDING; // Mark as sending
    // Note: We ignore onlineUserOnly and offlinePushInfo for now as Tox doesn't support them directly.
    if (onlineUserOnly) {
        V2TIM_LOG(kWarning, "SendMessage: onlineUserOnly flag is not supported by tim2tox.");
    }
    if (!offlinePushInfo.title.Empty() || !offlinePushInfo.desc.Empty()) {
         V2TIM_LOG(kWarning, "SendMessage: offlinePushInfo is not supported by tim2tox.");
    }

    // --- Get Lower-Level Manager Instance ---
    V2TIMManagerImpl* manager = V2TIMManagerImpl::GetInstance();
    if (!manager) {
         V2TIM_LOG(kError, "SendMessage failed: V2TIMManagerImpl instance is null.");
         if (callback) callback->OnError(ERR_SDK_NOT_INITIALIZED, "V2TIMManagerImpl instance is null");
         message.status = V2TIM_MSG_STATUS_SEND_FAIL;
         return "";
    }

    // --- Extract Payload and Call Appropriate Send Method ---
    V2TIMString sentMsgID = ""; // Store the ID returned by the lower-level send

    // Check if elemList has at least one element
    if (message.elemList.Size() == 0) {
        V2TIM_LOG(kError, "SendMessage failed: Message has no elements.");
        if (callback) callback->OnError(ERR_INVALID_PARAMETERS, "Message has no elements");
        message.status = V2TIM_MSG_STATUS_SEND_FAIL;
        return "";
    }

    V2TIMElem* elem = message.elemList[0];
    if (!elem) {
        V2TIM_LOG(kError, "SendMessage failed: First element is null.");
        if (callback) callback->OnError(ERR_INVALID_PARAMETERS, "First element is null");
        message.status = V2TIM_MSG_STATUS_SEND_FAIL;
        return "";
    }

    switch (elem->elemType) {
        case V2TIM_ELEM_TYPE_TEXT: {
            V2TIMTextElem* textElem = static_cast<V2TIMTextElem*>(elem);
            if (isC2C) {
                sentMsgID = manager->SendC2CTextMessage(textElem->text, receiver, callback);
            } else { // isGroup
                sentMsgID = manager->SendGroupTextMessage(textElem->text, groupID, priority, callback);
            }
            break;
        }
        case V2TIM_ELEM_TYPE_CUSTOM: {
             // TODO: Implement SendC2CCustomMessage and SendGroupCustomMessage in V2TIMManagerImpl
             V2TIM_LOG(kError, "SendMessage failed: Sending Custom messages not yet fully implemented in V2TIMManagerImpl.");
             if (callback) callback->OnError(ERR_SDK_NOT_SUPPORTED, "Sending Custom messages not yet implemented");
             sentMsgID = ""; // Indicate immediate failure
             break;
            // if (isC2C) {
            //     V2TIMCustomElem* customElem = static_cast<V2TIMCustomElem*>(elem);
            //     sentMsgID = manager->SendC2CCustomMessage(customElem->data, receiver, callback);
            // } else { // isGroup
            //     V2TIMCustomElem* customElem = static_cast<V2TIMCustomElem*>(elem);
            //     sentMsgID = manager->SendGroupCustomMessage(customElem->data, groupID, priority, callback);
            // }
        }
        // TODO: Add cases for other elem types (Image, File, etc.) once creation and sending are supported.
        default: {
             V2TIM_LOG(kError, "SendMessage failed: Unsupported message element type: %d", elem->elemType);
             if (callback) callback->OnError(ERR_SDK_NOT_SUPPORTED, "Unsupported message element type for sending");
             sentMsgID = ""; // Indicate immediate failure
             break;
        }
    }

    // --- Final Status Update and Return ---
    if (sentMsgID.Empty()) {
        // The send call failed immediately (e.g., invalid params, not implemented)
        message.status = V2TIM_MSG_STATUS_SEND_FAIL;
        // callback->OnError should have been called by the failed function or above check
    } else {
        // Send initiated, status remains SENDING. The callback will indicate final success/failure.
        // We return the original message ID generated during creation.
    }

    return message.msgID; // Return the client-generated message ID
}

// --- Other Methods (Placeholders) ---
void V2TIMMessageManagerImpl::SetC2CReceiveMessageOpt(const V2TIMStringVector& userIDList, V2TIMReceiveMessageOpt opt, V2TIMCallback* callback) { ReportNotImplemented(callback); }
void V2TIMMessageManagerImpl::GetC2CReceiveMessageOpt(const V2TIMStringVector& userIDList, V2TIMValueCallback<V2TIMReceiveMessageOptInfoVector>* callback) { ReportNotImplemented(callback); }
void V2TIMMessageManagerImpl::SetGroupReceiveMessageOpt(const V2TIMString& groupID, V2TIMReceiveMessageOpt opt, V2TIMCallback* callback) { ReportNotImplemented(callback); }
void V2TIMMessageManagerImpl::SetAllReceiveMessageOpt(V2TIMReceiveMessageOpt opt, int32_t startHour, int32_t startMinute, int32_t startSecond, uint32_t duration, V2TIMCallback* callback) { ReportNotImplemented(callback); }
void V2TIMMessageManagerImpl::SetAllReceiveMessageOpt(V2TIMReceiveMessageOpt opt, uint32_t startTimeStamp, uint32_t duration, V2TIMCallback* callback) { ReportNotImplemented(callback); }
void V2TIMMessageManagerImpl::GetAllReceiveMessageOpt(V2TIMValueCallback<V2TIMReceiveMessageOptInfo>* callback) { ReportNotImplemented(callback); }
void V2TIMMessageManagerImpl::GetHistoryMessageList(const V2TIMMessageListGetOption& option, V2TIMValueCallback<V2TIMMessageVector>* callback) { ReportNotImplemented(callback); }
void V2TIMMessageManagerImpl::RevokeMessage(const V2TIMMessage& message, V2TIMCallback* callback) { ReportNotImplemented(callback); }
void V2TIMMessageManagerImpl::ModifyMessage(const V2TIMMessage& message, V2TIMCompleteCallback<V2TIMMessage>* callback) { ReportNotImplemented(callback); }
void V2TIMMessageManagerImpl::DeleteMessages(const V2TIMMessageVector& messages, V2TIMCallback* callback) { ReportNotImplemented(callback); }
void V2TIMMessageManagerImpl::ClearC2CHistoryMessage(const V2TIMString& userID, V2TIMCallback* callback) { ReportNotImplemented(callback); }
void V2TIMMessageManagerImpl::ClearGroupHistoryMessage(const V2TIMString& groupID, V2TIMCallback* callback) { ReportNotImplemented(callback); }
V2TIMString V2TIMMessageManagerImpl::InsertGroupMessageToLocalStorage(V2TIMMessage &message, const V2TIMString &groupID, const V2TIMString &sender, V2TIMValueCallback<V2TIMMessage> *callback) { 
    ReportNotImplemented(callback); 
    return "";
}
V2TIMString V2TIMMessageManagerImpl::InsertC2CMessageToLocalStorage(V2TIMMessage &message, const V2TIMString &userID, const V2TIMString &sender, V2TIMValueCallback<V2TIMMessage> *callback) { 
    ReportNotImplemented(callback); 
    return "";
}
void V2TIMMessageManagerImpl::FindMessages(const V2TIMStringVector& messageIDList, V2TIMValueCallback<V2TIMMessageVector>* callback) { ReportNotImplemented(callback); }
void V2TIMMessageManagerImpl::SearchLocalMessages(const V2TIMMessageSearchParam& searchParam, V2TIMValueCallback<V2TIMMessageSearchResult>* callback) { ReportNotImplemented(callback); }
void V2TIMMessageManagerImpl::SearchCloudMessages(const V2TIMMessageSearchParam& searchParam, V2TIMValueCallback<V2TIMMessageSearchResult>* callback) { ReportNotImplemented(callback); }
void V2TIMMessageManagerImpl::SendMessageReadReceipts(const V2TIMMessageVector& messageList, V2TIMCallback* callback) { ReportNotImplemented(callback); }
void V2TIMMessageManagerImpl::GetMessageReadReceipts(const V2TIMMessageVector& messageList, V2TIMValueCallback<V2TIMMessageReceiptVector>* callback) { ReportNotImplemented(callback); }
void V2TIMMessageManagerImpl::GetGroupMessageReadMemberList(const V2TIMMessage& message, V2TIMGroupMessageReadMembersFilter filter, uint64_t nextSeq, uint32_t count, V2TIMValueCallback<V2TIMGroupMessageReadMemberList>* callback) { ReportNotImplemented(callback); }
void V2TIMMessageManagerImpl::MarkC2CMessageAsRead(const V2TIMString& userID, V2TIMCallback* callback) { ReportNotImplemented(callback); }
void V2TIMMessageManagerImpl::MarkGroupMessageAsRead(const V2TIMString& groupID, V2TIMCallback* callback) { ReportNotImplemented(callback); }
void V2TIMMessageManagerImpl::MarkAllMessageAsRead(V2TIMCallback *callback) { ReportNotImplemented(callback); }
void V2TIMMessageManagerImpl::SetMessageExtensions(const V2TIMMessage& message, const V2TIMMessageExtensionVector& extensions, V2TIMValueCallback<V2TIMMessageExtensionResultVector>* callback) { ReportNotImplemented(callback); }
void V2TIMMessageManagerImpl::GetMessageExtensions(const V2TIMMessage& message, V2TIMValueCallback<V2TIMMessageExtensionVector>* callback) { ReportNotImplemented(callback); }
void V2TIMMessageManagerImpl::DeleteMessageExtensions(const V2TIMMessage& message, const V2TIMStringVector& keys, V2TIMValueCallback<V2TIMMessageExtensionResultVector>* callback) { ReportNotImplemented(callback); }
void V2TIMMessageManagerImpl::AddMessageReaction(const V2TIMMessage& message, const V2TIMString& reactionID, V2TIMCallback* callback) { ReportNotImplemented(callback); }
void V2TIMMessageManagerImpl::RemoveMessageReaction(const V2TIMMessage& message, const V2TIMString& reactionID, V2TIMCallback* callback) { ReportNotImplemented(callback); }
void V2TIMMessageManagerImpl::GetMessageReactions(const V2TIMMessageVector& messageList, uint32_t maxUserCountPerReaction, V2TIMValueCallback<V2TIMMessageReactionResultVector>* callback) { ReportNotImplemented(callback); }
void V2TIMMessageManagerImpl::GetAllUserListOfMessageReaction(const V2TIMMessage& message, const V2TIMString& reactionID, uint32_t nextSeq, uint32_t count, V2TIMValueCallback<V2TIMMessageReactionUserResult>* callback) { ReportNotImplemented(callback); }
void V2TIMMessageManagerImpl::TranslateText(const V2TIMStringVector& sourceTextList, const V2TIMString& sourceLanguage, const V2TIMString& targetLanguage, V2TIMValueCallback<V2TIMStringToV2TIMStringMap>* callback) { ReportNotImplemented(callback); }
void V2TIMMessageManagerImpl::PinGroupMessage(const V2TIMString& groupID, const V2TIMMessage& message, bool isPinned, V2TIMCallback* callback) { ReportNotImplemented(callback); }
void V2TIMMessageManagerImpl::GetPinnedGroupMessageList(const V2TIMString& groupID, V2TIMValueCallback<V2TIMMessageVector>* callback) { ReportNotImplemented(callback); }
void V2TIMMessageManagerImpl::DownloadMergerMessage(const V2TIMMessage &message, V2TIMValueCallback<V2TIMMessageVector> *callback) { ReportNotImplemented(callback); }

// --- Internal method to notify listeners ---
void V2TIMMessageManagerImpl::NotifyAdvancedListenersReceivedMessage(const V2TIMMessage& message) {
    std::vector<V2TIMAdvancedMsgListener*> listeners_to_notify;
    {
        std::lock_guard<std::mutex> lock(listener_mutex_);
        listeners_to_notify.assign(listeners_.begin(), listeners_.end());
    }

    for (V2TIMAdvancedMsgListener* listener : listeners_to_notify) {
        if (listener) {
            // TODO: Add more checks/details if needed
            listener->OnRecvNewMessage(message);
        }
    }
}