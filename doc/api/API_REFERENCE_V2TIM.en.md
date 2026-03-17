# Tim2Tox API Reference — V2TIM C++
> Language: [Chinese](API_REFERENCE_V2TIM.md) | [English](API_REFERENCE_V2TIM.en.md)

This document is the V2TIM C++ section of [API_REFERENCE.en.md](API_REFERENCE.en.md).

## V2TIM C++ API

Tim2Tox implements a complete V2TIM API and is fully compatible with the V2TIM interface of Tencent Cloud IM SDK. All APIs are defined in header files in the `include/` directory.

### V2TIMManager

The core manager provides functions such as SDK initialization, login and logout, and obtaining other managers.

**Header file**: `include/V2TIMManager.h`

#### Initialization

```cpp
// Get singleton instance
static V2TIMManager* GetInstance();

// Initialize SDK
virtual bool InitSDK(uint32_t sdkAppID, const V2TIMSDKConfig& config) = 0;

// Deinitialize SDK
virtual void UnInitSDK() = 0;

// Get SDK version
virtual V2TIMString GetVersion() = 0;

// Get server time
virtual int64_t GetServerTime() = 0;
```

#### Listener management

```cpp
// Add SDK monitoring
virtual void AddSDKListener(V2TIMSDKListener* listener) = 0;

// Remove SDK monitoring
virtual void RemoveSDKListener(V2TIMSDKListener* listener) = 0;
```

#### Login and logout

```cpp
// Log in
virtual void Login(const V2TIMString& userID, const V2TIMString& userSig, V2TIMCallback* callback) = 0;

// Sign out
virtual void Logout(V2TIMCallback* callback) = 0;

// Get the current logged in user ID
virtual V2TIMString GetLoginUser() = 0;
```

#### Get Manager

```cpp
// Get message manager
virtual V2TIMMessageManager* GetMessageManager() = 0;

// Get group manager
virtual V2TIMGroupManager* GetGroupManager() = 0;

// Get session manager
virtual V2TIMConversationManager* GetConversationManager() = 0;

// Get friend manager
virtual V2TIMFriendshipManager* GetFriendshipManager() = 0;

// Get signaling manager
virtual V2TIMSignalingManager* GetSignalingManager() = 0;

// Get community manager
virtual V2TIMCommunityManager* GetCommunityManager() = 0;

// Get offline push manager
virtual V2TIMOfflinePushManager* GetOfflinePushManager() = 0;
```

### V2TIMMessageManager

Message manager provides functions such as message creation, sending, receiving, and querying.

**Header file**: `include/V2TIMMessageManager.h`

#### Listener

```cpp
// Add advanced message listening
virtual void AddAdvancedMsgListener(V2TIMAdvancedMsgListener* listener) = 0;

// Remove advanced message listening
virtual void RemoveAdvancedMsgListener(V2TIMAdvancedMsgListener* listener) = 0;
```

#### Create message

```cpp
// Create text message
virtual V2TIMMessage CreateTextMessage(const V2TIMString& text) = 0;

// Create a custom message
virtual V2TIMMessage CreateCustomMessage(const V2TIMBuffer& data) = 0;
virtual V2TIMMessage CreateCustomMessage(const V2TIMBuffer& data, const V2TIMString& description, const V2TIMString& extension) = 0;

// Create picture message
virtual V2TIMMessage CreateImageMessage(const V2TIMString& imagePath) = 0;

// Create a voice message
virtual V2TIMMessage CreateSoundMessage(const V2TIMString& soundPath, uint32_t duration) = 0;

// Create a video message
virtual V2TIMMessage CreateVideoMessage(const V2TIMString& videoFilePath, const V2TIMString& type, uint32_t duration, const V2TIMString& snapshotPath) = 0;

// Create file message
virtual V2TIMMessage CreateFileMessage(const V2TIMString& filePath, const V2TIMString& fileName) = 0;

// Create location message
virtual V2TIMMessage CreateLocationMessage(const V2TIMString& desc, double longitude, double latitude) = 0;

// Create an emoticon message
virtual V2TIMMessage CreateFaceMessage(uint32_t index, const V2TIMBuffer& data) = 0;

// Create merge message
virtual V2TIMMessage CreateMergerMessage(const V2TIMMessageVector& messageList, const V2TIMString& title, const V2TIMStringVector& abstractList, const V2TIMStringVector& compatibleText) = 0;
```

#### Send message

```cpp
// Send message
virtual void SendMessage(V2TIMMessage& message, const V2TIMString& receiver, V2TIMConversationType conversationType, V2TIMMessagePriority priority, V2TIMSendCallback* callback) = 0;

// Send message (with cloud custom data)
virtual void SendMessage(V2TIMMessage& message, const V2TIMString& receiver, V2TIMConversationType conversationType, V2TIMMessagePriority priority, const V2TIMBuffer& cloudCustomData, V2TIMSendCallback* callback) = 0;
```

#### Message Query

```cpp
// Find messages
virtual void FindMessages(const V2TIMStringVector& messageIDList, V2TIMValueCallback<V2TIMMessageVector>* callback) = 0;

// Get historical news
virtual void GetHistoryMessageList(const V2TIMString& conversationID, V2TIMConversationType conversationType, const V2TIMMessageGetHistoryMessageListParam& param, V2TIMValueCallback<V2TIMMessageListResult>* callback) = 0;
```

#### Message operation

```cpp
// Withdraw message
virtual void RevokeMessage(const V2TIMMessage& message, V2TIMCallback* callback) = 0;

// Mark message as read
virtual void MarkMessageAsRead(const V2TIMString& conversationID, V2TIMConversationType conversationType, V2TIMCallback* callback) = 0;

// Delete message
virtual void DeleteMessages(const V2TIMStringVector& messageIDList, V2TIMCallback* callback) = 0;
```

### V2TIMFriendshipManager

The friend manager provides functions such as adding, deleting, and querying friends.

**Header file**: `include/V2TIMFriendshipManager.h`

#### Listener

```cpp
// Add friend to monitor
virtual void AddFriendListener(V2TIMFriendshipListener* listener) = 0;

// Remove friend monitoring
virtual void RemoveFriendListener(V2TIMFriendshipListener* listener) = 0;
```

#### Friends list

```cpp
// Get friends list
virtual void GetFriendList(V2TIMValueCallback<V2TIMFriendInfoVector>* callback) = 0;

// Get specified friend information
virtual void GetFriendsInfo(const V2TIMStringVector& userIDList, V2TIMValueCallback<V2TIMFriendInfoResultVector>* callback) = 0;

// Set up designated friend information
virtual void SetFriendInfo(const V2TIMFriendInfo& info, V2TIMCallback* callback) = 0;
```

#### Friend operation

```cpp
// Add friends
virtual void AddFriend(const V2TIMFriendAddApplication& application, V2TIMValueCallback<V2TIMFriendOperationResult>* callback) = 0;

// delete friend
virtual void DeleteFromFriendList(const V2TIMStringVector& userIDList, V2TIMFriendType deleteType, V2TIMValueCallback<V2TIMFriendOperationResultVector>* callback) = 0;

// Check friend relationships
virtual void CheckFriend(const V2TIMStringVector& userIDList, V2TIMFriendType checkType, V2TIMValueCallback<V2TIMFriendCheckResultVector>* callback) = 0;
```

#### Friend application

```cpp
// Get friend application list
virtual void GetFriendApplicationList(V2TIMValueCallback<V2TIMFriendApplicationResult>* callback) = 0;

// Accept friend request
virtual void AcceptFriendApplication(const V2TIMFriendApplication& application, V2TIMFriendResponseType responseType, V2TIMValueCallback<V2TIMFriendOperationResult>* callback) = 0;

// Reject friend request
virtual void RefuseFriendApplication(const V2TIMFriendApplication& application, V2TIMValueCallback<V2TIMFriendOperationResult>* callback) = 0;
```

### V2TIMGroupManager

Group manager provides group creation, joining, management and other functions.

**Header file**: `include/V2TIMGroupManager.h`

#### Listener

```cpp
// Add group monitoring
virtual void AddGroupListener(V2TIMGroupListener* listener) = 0;

// Remove group monitoring
virtual void RemoveGroupListener(V2TIMGroupListener* listener) = 0;
```

#### Group operations

```cpp
// Create group
virtual void CreateGroup(const V2TIMGroupInfo& info, const V2TIMCreateGroupMemberInfoVector& memberList, V2TIMValueCallback<V2TIMString>* callback) = 0;
```

**Parameter description**:
- `info.groupType`: Group type
  - `"group"` - Use new API (`tox_group_new`), support `chat_id` persistence, recommended
  - `"conference"` - uses old API (`tox_conference_new`), relies on savedata recovery, for compatibility only
  - If not specified, defaults to `"group"` type
- `info.groupID`: Group ID (optional, will be automatically generated if not provided)
- `info.groupName`: Group name
- `memberList`: Initial member list (optional)

**BEHAVIOR**:
- If `groupType == "group"`: call `tox_group_new`, get and store `chat_id`
- if `groupType == "conference"`: calls `tox_conference_new`, does not store `chat_id`
- Regardless of the type, `groupType` will be stored in persistent storage

```cpp
// Get the list of joined groups
virtual void GetJoinedGroupList(V2TIMValueCallback<V2TIMGroupInfoVector>* callback) = 0;

// Get group information
virtual void GetGroupsInfo(const V2TIMStringVector& groupIDList, V2TIMValueCallback<V2TIMGroupInfoResultVector>* callback) = 0;

// Modify group information
virtual void SetGroupInfo(const V2TIMGroupInfo& info, V2TIMCallback* callback) = 0;
```

#### Group Member Management

```cpp
// Get the list of group members
virtual void GetGroupMemberList(const V2TIMString& groupID, V2TIMGroupMemberFilter filter, const V2TIMString& nextSeq, V2TIMValueCallback<V2TIMGroupMemberInfoResult>* callback) = 0;

// Get the specified group member information
virtual void GetGroupMembersInfo(const V2TIMString& groupID, const V2TIMStringVector& memberList, V2TIMValueCallback<V2TIMGroupMemberFullInfoVector>* callback) = 0;

// Modify the specified group member information
virtual void SetGroupMemberInfo(const V2TIMString& groupID, const V2TIMString& userID, const V2TIMGroupMemberInfo& info, V2TIMCallback* callback) = 0;
```

### V2TIMConversationManager

Session manager provides session query, deletion and other functions.

**Header file**: `include/V2TIMConversationManager.h`

#### Listener

```cpp
//Add session listener
virtual void AddConversationListener(V2TIMConversationListener* listener) = 0;

//Remove session monitoring
virtual void RemoveConversationListener(V2TIMConversationListener* listener) = 0;
```

#### Conversation Operations

```cpp
// Get session list
virtual void GetConversationList(const V2TIMConversationListFilter& filter, const V2TIMString& nextSeq, uint32_t count, V2TIMValueCallback<V2TIMConversationResult>* callback) = 0;

// Get the specified session
virtual void GetConversation(const V2TIMString& conversationID, V2TIMConversationType conversationType, V2TIMValueCallback<V2TIMConversation>* callback) = 0;

// Delete session
virtual void DeleteConversation(const V2TIMString& conversationID, V2TIMConversationType conversationType, V2TIMCallback* callback) = 0;

//Set session draft
virtual void SetConversationDraft(const V2TIMString& conversationID, V2TIMConversationType conversationType, const V2TIMString& draftText, V2TIMCallback* callback) = 0;
```

### V2TIMSignalingManager

Signaling manager provides functions such as audio and video call invitations.

**Header file**: `include/V2TIMSignalingManager.h`

#### Listener

```cpp
//Add signaling monitoring
virtual void AddSignalingListener(V2TIMSignalingListener* listener) = 0;

//Remove signaling monitoring
virtual void RemoveSignalingListener(V2TIMSignalingListener* listener) = 0;
```

#### Signaling Operations

```cpp
// Invitation (1 to 1)
virtual void Invite(const V2TIMString& invitee, const V2TIMString& data, bool onlineUserOnly, int timeout, V2TIMValueCallback<V2TIMString>* callback) = 0;

//Group invitation
virtual void InviteInGroup(const V2TIMString& groupID, const V2TIMStringVector& inviteeList, const V2TIMString& data, bool onlineUserOnly, int timeout, V2TIMValueCallback<V2TIMString>* callback) = 0;

// Cancel invitation
virtual void Cancel(const V2TIMString& inviteID, const V2TIMString& data, V2TIMCallback* callback) = 0;

//Accept invitation
virtual void Accept(const V2TIMString& inviteID, const V2TIMString& data, V2TIMCallback* callback) = 0;

// Decline invitation
virtual void Reject(const V2TIMString& inviteID, const V2TIMString& data, V2TIMCallback* callback) = 0;
```

### V2TIMCommunityManager

Community manager provides topics, permission group management and other functions.

**Header file**: `include/V2TIMCommunityManager.h`

#### Listener

```cpp
//Add community monitoring
virtual void AddCommunityListener(V2TIMCommunityListener* listener) = 0;

//Remove community monitoring
virtual void RemoveCommunityListener(V2TIMCommunityListener* listener) = 0;
```

#### Topic Operations

```cpp
//Create topic
virtual void CreateTopicInCommunity(const V2TIMString& groupID, const V2TIMTopicInfo& topicInfo, V2TIMValueCallback<V2TIMString>* callback) = 0;

// Delete topic
virtual void DeleteTopicFromCommunity(const V2TIMString& groupID, const V2TIMStringVector& topicIDList, V2TIMValueCallback<V2TIMTopicOperationResultVector>* callback) = 0;

//Modify topic information
virtual void SetTopicInfo(const V2TIMTopicInfo& topicInfo, V2TIMCallback* callback) = 0;

// Get the topic list
virtual void GetTopicInfoList(const V2TIMString& groupID, const V2TIMStringVector& topicIDList, V2TIMValueCallback<V2TIMTopicInfoResultVector>* callback) = 0;
```
