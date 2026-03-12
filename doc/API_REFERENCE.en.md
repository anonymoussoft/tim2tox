# Tim2Tox API Reference
> Language: [Chinese](API_REFERENCE.md) | [English](API_REFERENCE.en.md)


This document provides a complete API reference for Tim2Tox, including the C++ V2TIM API, C FFI interface, and Dart package API.

## Contents

- [V2TIM C++ API](#v2tim-c-api)
- [C FFI Interface](#c-ffi-interface)
- [Dart Package API](#dart-package-api)
- [Data Types](#data-types)

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

### V2TIMFirendshipManager

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
```

**Return information**:
- `V2TIMGroupInfo.groupType`: Group type ("group" or "conference")
- `V2TIMGroupInfo.notification`: Group announcement (only supported by Group type, obtained from `tox_group_get_topic`)

//Modify group information
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
virtual void AddSignalingListener(V2TIMSignalingListener* listener) = 0;//Remove signaling monitoring
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

## C FFI Interface

The C FFI interface provides C language bindings for Dart FFI calls. All interfaces are defined in `ffi/tim2tox_ffi.h`.

### Initialization and Cleanup

```c
//Initialize SDK
int tim2tox_ffi_init(void);

//Deinitialize SDK
void tim2tox_ffi_uninit(void);

//Set the file receiving directory
int tim2tox_ffi_set_file_recv_dir(const char* dir_path);
```

### Login

```c
// Login
int tim2tox_ffi_login(const char* user_id, const char* user_sig);

// Get the current logged in user ID
int tim2tox_ffi_get_login_user(char* buffer, int buffer_len);
```

### Messages

```c
//Send C2C text message
int tim2tox_ffi_send_c2c_text(const char* user_id, const char* text);

//Send C2C custom message
int tim2tox_ffi_send_c2c_custom(const char* user_id, const unsigned char* data, int data_len);

//Send group custom message
int tim2tox_ffi_send_group_custom(const char* group_id, const unsigned char* data, int data_len);

//Poll for received text messages
int tim2tox_ffi_poll_text(char* buffer, int buffer_len);

//Poll for received custom messages
int tim2tox_ffi_poll_custom(unsigned char* buffer, int buffer_len);
```

### Friendship

```c
//Add friend
int tim2tox_ffi_add_friend(const char* user_id, const char* wording);

// Get the friends list
int tim2tox_ffi_get_friend_list(char* buffer, int buffer_len);

// Get the friend application list
int tim2tox_ffi_get_friend_applications(char* buffer, int buffer_len);

//Accept friend request
int tim2tox_ffi_accept_friend(const char* user_id);

// Delete friends
int tim2tox_ffi_delete_friend(const char* user_id);

//Set input status
int tim2tox_ffi_set_typing(const char* user_id, int typing_on);
```

### Groups

```c
//Create group
// group_type: "group" (new API) or "conference" (old API)
int tim2tox_ffi_create_group(const char* group_name, const char* group_type, char* out_group_id, int out_len);

// Join the group (fixed use group API)
int tim2tox_ffi_join_group(const char* group_id, const char* request_msg);//Send group text message
int tim2tox_ffi_send_group_text(const char* group_id, const char* text);

//Storage group type
int tim2tox_ffi_set_group_type(const char* group_id, const char* group_type);

// Get the group type
int tim2tox_ffi_get_group_type_from_storage(const char* group_id, char* out_group_type, int out_len);
```

### File Transfer

```c
//Send file
int tim2tox_ffi_send_file(const char* user_id, const char* file_path);

//Control file transfer
int tim2tox_ffi_file_control(const char* user_id, uint32_t file_number, int control);
```

### Signaling (Calling)

```c
//Add signaling monitoring
int tim2tox_ffi_signaling_add_listener(
    tim2tox_signaling_invitation_callback_t on_invitation,
    tim2tox_signaling_cancel_callback_t on_cancel,
    tim2tox_signaling_accept_callback_t on_accept,
    tim2tox_signaling_reject_callback_t on_reject,
    tim2tox_signaling_timeout_callback_t on_timeout,
    void* user_data
);

//Invite users (1 to 1)
int tim2tox_ffi_signaling_invite(const char* invitee, const char* data, int online_user_only, int timeout, char* out_invite_id, int out_invite_id_len);

//Group invitation
int tim2tox_ffi_signaling_invite_in_group(const char* group_id, const char* invitee_list, const char* data, int online_user_only, int timeout, char* out_invite_id, int out_invite_id_len);

// Cancel invitation
int tim2tox_ffi_signaling_cancel(const char* invite_id, const char* data);

//Accept invitation
int tim2tox_ffi_signaling_accept(const char* invite_id, const char* data);

// Decline invitation
int tim2tox_ffi_signaling_reject(const char* invite_id, const char* data);
```

### Audio and Video (ToxAV)

```c
//Initialize ToxAV
int tim2tox_ffi_av_initialize(int64_t instance_id);

// Close ToxAV
void tim2tox_ffi_av_shutdown(int64_t instance_id);

// Iterate ToxAV (needs to be called periodically in the main loop)
void tim2tox_ffi_av_iterate(int64_t instance_id);

// Initiate a call
int tim2tox_ffi_av_start_call(int64_t instance_id, uint32_t friend_number, uint32_t audio_bit_rate, uint32_t video_bit_rate);

// answer the call
int tim2tox_ffi_av_answer_call(int64_t instance_id, uint32_t friend_number, uint32_t audio_bit_rate, uint32_t video_bit_rate);

// end call
int tim2tox_ffi_av_end_call(int64_t instance_id, uint32_t friend_number);

//Mute/unmute audio
int tim2tox_ffi_av_mute_audio(int64_t instance_id, uint32_t friend_number, int mute);

//Hide/show video
int tim2tox_ffi_av_mute_video(int64_t instance_id, uint32_t friend_number, int hide);

//Send audio frame
int tim2tox_ffi_av_send_audio_frame(int64_t instance_id, uint32_t friend_number, const int16_t* pcm, size_t sample_count, uint8_t channels, uint32_t sampling_rate);

//Send video frame (YUV420 format)
int tim2tox_ffi_av_send_video_frame(int64_t instance_id, uint32_t friend_number, uint16_t width, uint16_t height, const uint8_t* y, const uint8_t* u, const uint8_t* v, int32_t y_stride, int32_t u_stride, int32_t v_stride);

//Set audio and video bit rate
int tim2tox_ffi_av_set_audio_bit_rate(int64_t instance_id, uint32_t friend_number, uint32_t audio_bit_rate);
int tim2tox_ffi_av_set_video_bit_rate(int64_t instance_id, uint32_t friend_number, uint32_t video_bit_rate);// Register ToxAV callback
void tim2tox_ffi_av_set_call_callback(int64_t instance_id, tim2tox_av_call_callback_t callback, void* user_data);
void tim2tox_ffi_av_set_call_state_callback(int64_t instance_id, tim2tox_av_call_state_callback_t callback, void* user_data);
void tim2tox_ffi_av_set_audio_receive_callback(int64_t instance_id, tim2tox_av_audio_receive_callback_t callback, void* user_data);
void tim2tox_ffi_av_set_video_receive_callback(int64_t instance_id, tim2tox_av_video_receive_callback_t callback, void* user_data);
```

### IRC Channel Bridge

```c
// Connect to the IRC server and join the channel
int tim2tox_ffi_irc_connect_channel(const char* server, int port, const char* channel, const char* password, const char* group_id, const char* sasl_username, const char* sasl_password, int use_ssl, const char* custom_nickname);

//Disconnect IRC channel
int tim2tox_ffi_irc_disconnect_channel(const char* channel);

//Send message to IRC channel
int tim2tox_ffi_irc_send_message(const char* channel, const char* message);

// Check if the IRC channel is connected
int tim2tox_ffi_irc_is_connected(const char* channel);
```

### Callback Mechanism

```c
//Event callback type
typedef void (*tim2tox_event_cb)(int event_type, const char* sender, const unsigned char* payload, int payload_len, void* user_data);

//Register event callback
void tim2tox_ffi_set_callback(tim2tox_event_cb cb, void* user_data);
```

## Dart Package API

The Dart package provides a high-level API, encapsulates FFI calls, and provides an easier-to-use interface.

### Tim2ToxFfi

The underlying FFI binding class maps directly to C FFI functions.

**File**: `dart/lib/ffi/tim2tox_ffi.dart`

```dart
//Open FFI library
static Tim2ToxFfi open()

//Set unified log file
void setLogFile(String path)

// Get the current instance ID
int getCurrentInstanceId()

// initialization
int init()

// Login
int login(String userID, String userSig)

//Poll native events
int pollText(int instanceId, List<int> buffer)

// Get logged in user
int getLoginUser(List<int> buffer)

//ToxAV
int avInitialize(int instanceId)
void avShutdown(int instanceId)
void avIterate(int instanceId)
```

### FfiChatService

Advanced service layer manages message history, polling, status, etc.

**File**: `dart/lib/service/ffi_chat_service.dart`

```dart
class FfiChatService {
  //Constructor
  FfiChatService({
    ExtendedPreferencesService? preferencesService,
    LoggerService? loggerService,
    BootstrapService? bootstrapService,
    MessageHistoryPersistence? messageHistoryPersistence,
    OfflineMessageQueuePersistence? offlineMessageQueuePersistence,
    String?historyContents,
    String? queueFilePath,
    String? fileRecvPath,
    String? avatarsPath,
  })

  // initialization
  Future<void> init({String? profileContents})

  // Login
  Future<void> login({required String userId, required String userSig})

  // Start polling
  Future<void> startPolling()

  //Send text message
  Future<String?> sendMessage(String peerId, String text)

  // message flow
  Stream<ChatMessage> get messages

  //Connection status stream
  Stream<bool> get connectionStatusStream

  // Get message history
  List<ChatMessage> getHistory(String id)

  //Set input status
  void setTyping(String userID, bool typing)
}
```

### Tim2ToxSdkPlatform

Implement the `TencentCloudChatSdkPlatform` interface to route UIKit SDK calls to tim2tox.

**File**: `dart/lib/sdk/tim2tox_sdk_platform.dart`

```dart
class Tim2ToxSdkPlatform extends TencentCloudChatSdkPlatform {
  //Constructor
  Tim2ToxSdkPlatform({
    required FfiChatService ffiService,
    EventBusProvider? eventBusProvider,
    ConversationManagerProvider? conversationManagerProvider,
    ExtendedPreferencesService? preferencesService,
  })

  // SDK initialization
  @override
  Future<bool> initSDK({...})

  // Login
  @override
  Future<V2TimCallback> login({...})

  // log out
  @override
  Future<V2TimCallback> logout()

  // send message
  @override
  Future<V2TimValueCallback<V2TimMessage>> sendMessage({...})// Get the friends list
  @override
  Future<V2TimValueCallback<List<V2TimFriendInfo>>> getFriendList()//Add friend
  @override
  Future<V2TimValueCallback<V2TimFriendOperationResult>> addFriend({...})
}
```

### Interface Definitions

Tim2Tox injects client dependencies through interfaces, and all interfaces are defined in the `dart/lib/interfaces/` directory.

#### ExtendedPreferencesService

Preferences service interface.

```dart
abstract class ExtendedPreferencesService {
  Future<Set<String>> getGroups();
  Future<void> setGroups(Set<String> groups);
  Future<String?> getFriendNickname(String friendId);
  Future<void> setFriendNickname(String friendId, String nickname);
  Future<({String host, int port, String pubkey})?> getCurrentBootstrapNode();
  Future<void> setCurrentBootstrapNode(String host, int port, String pubkey);
  Future<String?> getGroupChatId(String groupId);
  Future<void> setGroupChatId(String groupId, String chatId);
}
```

#### LoggerService

Log service interface.

```dart
abstract class LoggerService {
  void log(String message);
  void logError(String message, Object error, StackTrace stack);
  void logWarning(String message);
  void logDebug(String message);
}
```

#### BootstrapService

Bootstrap node service interface.

```dart
abstract class BootstrapService {
  Future<String?> getBootstrapHost();
  Future<int?> getBootstrapPort();
  Future<String?> getBootstrapPublicKey();
  Future<void> setBootstrapNode({
    required String host,
    required int port,
    required String publicKey,
  });
}
```

#### EventBusProvider

Event bus provider interface.

```dart
abstract class EventBusProvider {
  EventBus get eventBus;
}
```

#### ConversationManagerProvider

Session manager provider interface.

```dart
abstract class ConversationManagerProvider {
  Future<List<FakeConversation>> getConversationList();
  Future<void> setPinned(String conversationID, bool isPinned);
  Future<void> deleteConversation(String conversationID);
  Future<int> getTotalUnreadCount();
}
```

## Data Types

### V2TIMString

String type, compatible with C++ and Dart.

```cpp
class V2TIMString {
    const char* CString() const;
    // ...
};
```

### V2TIMBuffer

Binary data buffer.

```cpp
class V2TIMBuffer {
    const uint8_t* Data() const;
    size_t Size() const;
    // ...
};
```

### V2TIMMessage

message object.

```cpp
struct V2TIMMessage {
    V2TIMString msgID;
    int64_t timestamp;
    V2TIMString sender;
    V2TIMString userID;
    V2TIMString groupID;
    V2TIMElemVector elemList;
    // ...
};
```

### V2TIMFriendInfo

Friend information.

```cpp
struct V2TIMFriendInfo {
    V2TIMString userID;
    V2TIMString nickName;
    V2TIMString faceURL;
    V2TIMFriendType friendType;
    // ...
};
```

### V2TIMGroupInfo

Group information.

```cpp
struct V2TIMGroupInfo {
    V2TIMString groupID;
    V2TIMString groupName;
    V2TIMString groupType; // "group" (new API) or "conference" (old API)
    V2TIMString notification; // Group announcement (only supported by Group type, corresponding to tox_group_topic)
    V2TIMString introduction;
    // ...
};
```

**V2TIMGroupMemberFullInfo**:
```cpp
struct V2TIMGroupMemberFullInfo {
    V2TIMString userID;
    V2TIMString nickName; // User nickname (obtained from friend information)
    V2TIMString nameCard; // Group nickname (obtained from tox_group_peer_get_name, only supported by Group type)
    uint32_t role; // Role: OWNER, ADMIN, MEMBER
    // ...
};
```

## Callback Types

### V2TIMCallback

Universal callback interface.

```cpp
class V2TIMCallback {
    virtual void OnSuccess() = 0;
    virtual void OnError(int error_code, const V2TIMString& error_message) = 0;
};
```

### V2TIMValueCallback

Callback interface with return value.

```cpp
template<class T>
class V2TIMValueCallback {
    virtual void OnSuccess(const T& value) = 0;
    virtual void OnError(int error_code, const V2TIMString& error_message) = 0;
};
```

### V2TIMSendCallback

Message sending callback interface.

```cpp
class V2TIMSendCallback {
    virtual void OnSuccess(const V2TIMMessage& message) = 0;
    virtual void OnError(int error_code, const V2TIMString& error_message) = 0;
    virtual void OnProgress(uint32_t progress) = 0;
};
```

## Error Codes

Common error codes are defined in `include/V2TIMErrorCode.h`:

- `ERR_SUCC` (0): Success
- `ERR_INVALID_PARAMETERS` (6017): Invalid parameter
- `ERR_SDK_NOT_INITIALIZED` (6018): SDK not initialized
- `ERR_USER_SIG_EXPIRED` (6206): User signature expired
- `ERR_SDK_COMM_API_CALL_FREQUENCY_LIMIT` (7008): API call frequency limit

## Usage Examples

### C++ Usage Example

```cpp
# include <V2TIMManager.h>int main() {    //Initialize SDK
    V2TIMSDKConfig config;
    config.logLevel = V2TIM_LOG_DEBUG;
    V2TIMManager::GetInstance()->InitSDK(123456, config);
    
    // Login
    V2TIMManager::GetInstance()->Login(
        V2TIMString("user123"),
        V2TIMString("userSig"),
        new V2TIMCallback(
            []() { /* Success */ },
            [](int code, const V2TIMString& msg) { /* failed */ }
        )
    );
    
    // send message
    auto msgMgr = V2TIMManager::GetInstance()->GetMessageManager();
    V2TIMMessage msg = msgMgr->CreateTextMessage(V2TIMString("Hello"));
    msgMgr->SendMessage(
        msg,
        V2TIMString("friend123"),
        V2TIMConversationType::V2TIM_C2C,
        V2TIMMessagePriority::V2TIM_PRIORITY_NORMAL,
        new V2TIMSendCallback(/* ... */)
    );
    
    return 0;
}
```

### Dart Usage Example

```dart
import 'package:tim2tox_dart/tim2tox_dart.dart';

//Create service
final ffiService = FfiChatService(
  preferencesService: prefsAdapter,
  loggerService: loggerAdapter,
  bootstrapService: bootstrapAdapter,
);

// initialization
await ffiService.init();

// Login
await ffiService.login(userId: userID, userSig: userSig);

// send message
await ffiService.sendMessage(peerId, "Hello");

// Listen for messages
ffiService.messages.listen((message) {
  print('Received: ${message.text}');
});
```

## Related documents

- [Development Guide](DEVELOPMENT_GUIDE.en.md) - How to add new features and extensions
- [Tim2Tox Architecture](ARCHITECTURE.en.md) - Overall architecture design
- [Tim2Tox FFI compatibility layer](FFI_COMPAT_LAYER.en.md) - Dart* function compatibility layer description
