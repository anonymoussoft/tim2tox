# Tim2Tox FFI Compatibility Layer
> Language: [Chinese](FFI_COMPAT_LAYER.md) | [English](FFI_COMPAT_LAYER.en.md)


This document details the implementation of the Dart* function compatibility layer, including the callback mechanism, JSON message format, and implementation status.

## Contents

- [Overview](#overview)
- [Architecture Design](#architecture-design)
- [Callback Mechanism](#callback-mechanism)
- [JSON message format](#json-message-format)
- [Implementation status](#implementation-status)
- [User Guide](#user-guide)

## Overview

The Dart* function compatibility layer is the core component of the binary replacement solution. It implements the Dart* function interface that is fully compatible with the native IM SDK, allowing `NativeLibraryManager` in the Dart layer to seamlessly switch to the tim2tox backend without modifying any Dart code.

### Core Idea

1. **Exact match of function signatures**: The signatures of all Dart* functions are exactly the same as those defined in `native_imsdk_bindings_generated.dart`
2. **On-demand implementation**: Only implement the actually used functions (about 68), rather than all defined functions (about 230)
3. **Callback Bridging**: Send C++ events to the Dart layer via JSON messages and `Dart_PostCObject_DL`
4. **Binary replacement**: Just replace the dynamic library file, and the Dart layer code does not need to be modified at all.

### Advantages

- ✅ **Zero Dart code modification**: Dart layer code does not need to be modified at all
- ✅ **Significant workload reduction**: from 477 functions to 68 functions (85% reduction)
- ✅ **Full Compatibility**: Function signature and callback format exactly match the native SDK
- ✅ **Easy to Maintain**: Clear module division and code structure

## Architecture design

### Overall architecture

```
Dart layer (NativeLibraryManager)
    ↓ bindings.DartXXX(...)
    ↓ FFI dynamic symbol lookup
C++ layer (dart_compat_layer.cpp)
    ↓ DartXXX() function implementation
    ↓ JSON parsing + parameter conversion
    ↓ Call V2TIM SDK API
V2TIM SDK (tim2tox)
    ↓ Execute actual logic
    ↓ Callback (Listener)
C++ layer (Listener implementation)
    ↓ Convert to JSON message
    ↓ SendCallbackToDart()
Dart layer (ReceivePort)
    ↓ NativeLibraryManager._handleNativeMessage()
    ↓ Distribute to business code
```

### Core module

#### 1. Modular architecture

The Dart* function compatibility layer has been split into multiple functional modules to improve code maintainability:

**Module Structure**:
- `dart_compat_internal.h`: shared declarations and forward declarations
- `dart_compat_utils.cpp`: Utility functions and global variables (about 300 lines)
- `dart_compat_listeners.cpp`: Listener implementation and callback registration (about 1150 lines)
- `dart_compat_callbacks.cpp`: Callback class implementation (about 590 lines)
- `dart_compat_sdk.cpp`: SDK initialization and authentication (about 200 lines)
- `dart_compat_message.cpp`: Message related functions (about 1200 lines)
- `dart_compat_friendship.cpp`: Friend-related functions (about 630 lines)
- `dart_compat_conversation.cpp`: Session related functions (about 400 lines)
- `dart_compat_group.cpp`: Group related functions (about 900 lines)
- `dart_compat_user.cpp`: User related functions (about 550 lines)
- `dart_compat_signaling.cpp`: Signaling related functions (about 570 lines)
- `dart_compat_community.cpp`: Community-related functions (about 15 lines, to be improved)
- `dart_compat_other.cpp`: Other miscellaneous functions (about 15 lines, to be improved)
- `dart_compat_layer.cpp`: Main entry file (only header file, 29 lines)

**Total code size**: about 6764 lines (distributed in 13 module files)

**Main functions**:
- Export all actually used Dart* functions (about 131+ core functions)
- Implement callback setting function (storage user_data, register Listener)
- Implement business API functions (parse parameters, call V2TIM SDK)

#### 2. callback_bridge.cpp/h

Implement a callback bridging mechanism to send C++ events to the Dart layer.

**Main functions**:
- `DartInitDartApiDL`: Initialize Dart API
- `DartRegisterSendPort`: Register Dart SendPort
- `SendCallbackToDart`: Send callback message to Dart layer

#### 3. json_parser.cpp/h

Implement JSON message building and parsing tools.

**Main functions**:
- `BuildGlobalCallbackJson`: Build globalCallback JSON message
- `BuildApiCallbackJson`: Build apiCallback JSON message
- `ParseJsonString`: Parse JSON string
- `ExtractJsonValue`: Extract JSON values

#### 4. Listener implementation

Implement the V2TIM Listener interface to convert events into JSON messages.

**Main Listener**:
- `DartSDKListenerImpl`: SDK event monitoring
- `DartAdvancedMsgListenerImpl`: Message event monitoring
- `DartFriendshipListenerImpl`: Friend event monitoring
- `DartConversationListenerImpl`: Session event monitoring
- `DartGroupListenerImpl`: Group event monitoring
- `DartSignalingListenerImpl`: Signaling event monitoring
- `DartCommunityListenerImpl`: Community event monitoring

## Callback mechanism

### Initialization process

1. **Dart layer initialization Dart API**:
```dart
final result = bindings.DartInitDartApiDL(DartApiDL.initData);
```

2. **Dart layer registration SendPort**:
```dart
final receivePort = ReceivePort();
bindings.DartRegisterSendPort(receivePort.sendPort.nativePort);
```

3. **C++ layer storage SendPort**:
```cpp
void DartRegisterSendPort(int send_port) {
    g_dart_port = static_cast<Dart_Port>(send_port);
}
```

### Callback sending process

1. **C++ layer event trigger**:
```cpp
void DartAdvancedMsgListenerImpl::OnRecvNewMessage(const V2TIMMessage& message) {
    // Build a JSON message
    std::string json = BuildGlobalCallbackJson(
        GlobalCallbackType::ReceiveNewMessage,
        {{"json_message", MessageToJson(message)}}
    );
    
    // Send to Dart layer
    SendCallbackToDart("globalCallback", json, nullptr);
}
```

2. **Sent via Dart_PostCObject_DL**:
```cpp
void SendCallbackToDart(const char* callback_type, const std::string& json_data, void* user_data) {
    Dart_CObject cobj;
    cobj.type = Dart_CObject_kString;
    cobj.value.as_string = strdup(json_data.c_str());
    
    Dart_PostCObject_DL(g_dart_port, &cobj);
}
```

3. **Dart layer receives message**:
```dart
receivePort.listen((message) {
    final json = jsonDecode(message as String);
    _handleNativeMessage(json);
});
```

### Callback type

#### Global Callback

Used for event notification, format:

```json
{
  "callback": "globalCallback",
  "callbackType": 7,
  "json_message": "{...}",
  "user_data": "..."
}
```

#### API Callback

For API call results, format:

```json
{
  "callback": "apiCallback",
  "user_data": "...",
  "code": 0,
  "desc": "OK",
  "json_...": "{...}"
}
```

## JSON message format

### Global Callback Format

#### Receive new messages
```json
{
  "callback": "globalCallback",
  "callbackType": 7,
  "json_message": "{\"msgID\":\"1234567890_user123\",\"timestamp\":1234567890,\"sender\":\"user123\",\"text\":\"Hello\"}",
  "user_data": ""
}
```

#### Add friends

```json
{
  "callback": "globalCallback",
  "callbackType": 40,
  "json_friend_info_array": "[{\"userID\":\"user123\",\"nickName\":\"Alice\"}]",
  "user_data": ""
}
```

#### Group Tips

```json
{
  "callback": "globalCallback",
  "callbackType": 17,
  "json_group_tips_elem": "{\"type\":1,\"groupID\":\"group123\",\"memberList\":[{\"userID\":\"user123\"}]}",
  "user_data": ""
}
```

### API Callback Format

#### Login results

```json
{
  "callback": "apiCallback",
  "user_data": "login_123",
  "code": 0,
  "desc": "OK"
}
```

#### Get friend list results

```json
{
  "callback": "apiCallback",
  "user_data": "get_friend_list_456",
  "code": 0,
  "desc": "OK",
  "json_friend_info_array": "[{\"userID\":\"user123\",\"nickName\":\"Alice\"}]"
}
```

#### Send message result

```json
{
  "callback": "apiCallback",
  "user_data": "send_msg_789",
  "code": 0,
  "desc": "OK",
  "json_message": "{\"msgID\":\"1234567890_user123\",\"timestamp\":1234567890}"
}
```

### Field naming rules

JSON field names must match exactly what `NativeLibraryManager._handleGlobalCallback` expects:

- `json_message`: message object
- `json_friend_info_array`: Friend information array
- `json_group_tips_elem`: Group prompt element
- `json_conversation_array`: session array
- `json_signaling_info`: signaling information
- `json_topic_info`: Topic information

### Field compatibility processing

In order to be compatible with different versions of SDK or different calling methods, some functions support multiple field names:

#### `DartGetUsersInfo` function

**Implementation location**: `tim2tox/ffi/dart_compat_layer.cpp:5617-5656`

This function supports two field name formats:

1. **Standard format**: `get_user_profile_list_param_identifier_array`
   - Query parameters for standard user profiles

2. **SDK format**: `friendship_getprofilelist_param_identifier_array`
   - Used for SDK internal calls (such as friend information query)

**Implementation logic**:
```cpp
// Try the standard format first
std::string user_id_list_str = ExtractJsonValue(json_str, "get_user_profile_list_param_identifier_array");
if (user_id_list_str.empty()) {
    // If empty, fallback to SDK format
    user_id_list_str = ExtractJsonValue(json_str, "friendship_getprofilelist_param_identifier_array");
}
```

This ensures that no matter which field name is used, the function can correctly parse the parameters and improve compatibility.

## Implementation status

### Completed (Phase 1-2)

#### ✅ Infrastructure

- `DartInitDartApiDL`: Initialize Dart API
- `DartRegisterSendPort`: Register Dart SendPort
- `SendCallbackToDart`: Send callback message
- JSON parsing and building tools

#### ✅ Callback setting function (66)- SDK callbacks (8)
- Message callbacks (9)
- Session callbacks (8)
- Group callbacks (3)
- Friend callbacks (25)
- Signaling callbacks (6)
- Community callbacks (12)

#### ✅ Listener implementation

- ✅ `DartSDKListenerImpl`: Core method implementation (OnConnectSuccess, OnConnectFailed, OnKickedOffline, OnUserSigExpired, OnSelfInfoUpdated, OnUserStatusChanged, OnUserInfoChanged, OnLog)
- ✅ `DartAdvancedMsgListenerImpl`: Core method implementation (OnRecvNewMessage, OnRecvMessageModified, OnRecvMessageRevoked, OnRecvMessageReadReceipts, OnRecvMessageExtensionsChanged, OnRecvMessageExtensionsDeleted, OnRecvMessageReactionChanged, OnRecvAllMessageReceiveOptionChanged, OnRecvGroupPinnedMessageChanged)
- ✅ `DartFriendshipListenerImpl`: Core method implementation (OnFriendListAdded, OnFriendListDeleted, OnFriendInfoChanged, OnFriendApplicationListAdded, OnFriendApplicationListDeleted, OnFriendApplicationListRead, OnBlackListAdded, OnBlackListDeleted, OnFriendGroupCreated, OnFriendGroupDeleted, OnFriendGroupNameChanged, OnFriendsAddedToGroup, OnFriendsDeletedFromGroup, OnOfficialAccountSubscribed, OnOfficialAccountUnsubscribed, OnOfficialAccountDeleted, OnOfficialAccountInfoChanged, OnMyFollowingListChanged, OnMyFollowersListChanged, OnMutualFollowersListChanged)
- ✅ `DartConversationListenerImpl`: All methods implemented (OnConversationChanged, OnTotalUnreadMessageCountChanged, OnUnreadMessageCountChangedByFilter, OnConversationGroupCreated, OnConversationGroupDeleted, OnConversationGroupNameChanged, OnConversationsAddedToGroup, OnConversationsDeletedFromGroup)
- ✅ `DartGroupListenerImpl`: All methods implemented (OnGroupTipsEvent, OnGroupAttributeChanged)
- ✅ `DartSignalingListenerImpl`: All methods implemented (OnReceiveNewInvitation, OnInvitationCancelled, OnInviteeAccepted, OnInviteeRejected, OnInvitationTimeout, OnInvitationModified)
- ✅ `DartCommunityListenerImpl`: All methods implemented (OnTopicCreated, OnTopicDeleted, OnTopicChanged, OnReceiveTopicRESTCustomData)

### In progress (Phase 4)

#### ✅ Business API functions (~165/150+, ~98%)**Done** (~165 core functions):
- ✅ SDK basic functions: `DartInitSDK`, `DartUnitSDK`, `DartGetSDKVersion`, `DartGetServerTime`, `DartSetConfig`, `DartLogin`, `DartLogout`, `DartGetLoginUserID`, `DartGetLoginStatus`, `DartCallExperimentalAPI`, etc.
- ✅ Message function: `DartSendMessage`, `DartFindMessages`, `DartRevokeMessage`, `DartModifyMessage`, `DartDeleteMessages`, `DartGetHistoryMessageList`, `DartDownloadElemToPath`, `DartDownloadMergerMessage`, `DartSetLocalCustomData`, `DartSaveMessage`, `DartDeleteMessageFromLocalStorage`, `DartClearHistoryMessage`, `DartSearchLocalMessages`, `DartSearchCloudMessages`, `DartSendMessageReadReceipts`, `DartGetMessageReadReceipts`, `DartGetGroupMessageReadMemberList`, `DartSetMessageExtensions`, `DartGetMessageExtensions`, `DartDeleteMessageExtensions`, `DartAddMessageReaction`, `DartRemoveMessageReaction`, `DartGetMessageReactions`, `DartMarkC2CMessageAsRead`, `DartMarkGroupMessageAsRead`, `DartMarkAllMessageAsRead`, `DartTranslateText`, `DartConvertVoiceToText`, `DartPinGroupMessage`, `DartGetGroupPinnedMessageList`, `DartCreateTextMessage`, `DartCreateCustomMessage`, `DartCreateImageMessage`, `DartCreateSoundMessage`, `DartCreateVideoMessage`, `DartCreateFileMessage`, `DartCreateLocationMessage`, `DartCreateFaceMessage`, `DartCreateMergerMessage`, `DartCreateForwardMessage`, `DartCreateAtSignedGroupMessage`, `DartCreateTargetedGroupMessage`, etc.
- ✅ Friend functions: `DartGetFriendList`, `DartAddFriend`, `DartDeleteFromFriendList`, `DartGetFriendsInfo`, `DartSetFriendInfo`, `DartCheckFriend`, `DartGetFriendApplicationList`, `DartAcceptFriendApplication`, `DartRefuseFriendApplication`, `DartDeleteFriendApplication`, `DartCreateFriendGroup`, `DartGetFriendGroups`, `DartDeleteFriendGroup`, `DartRenameFriendGroup`, `DartAddFriendsToFriendGroup`, `DartDeleteFriendsFromFriendGroup`, `DartGetBlackList`, `DartAddToBlackList`, `DartDeleteFromBlackList`, `DartSubscribeOfficialAccount`, `DartUnsubscribeOfficialAccount`, `DartGetOfficialAccountsInfo`, `DartFollowUser`, `DartUnfollowUser`, `DartGetMyFollowingList`, `DartGetMyFollowersList`, `DartGetMutualFollowersList`, `DartGetUserFollowInfo`, `DartCheckFollowType`, etc.
- ✅ Group functions: `DartCreateGroup`, `DartJoinGroup`, `DartGetJoinedGroupList`, `DartQuitGroup`, `DartDismissGroup`, `DartGetGroupsInfo`, `DartSetGroupInfo`, `DartGetGroupMemberList`, `DartGetGroupOnlineMemberCount`, `DartSetGroupMemberInfo`, `DartGetGroupMembersInfo`, `DartMuteGroupMember`, `DartKickGroupMember`, `DartSetGroupMemberRole`, `DartTransferGroupOwner`, `DartInviteUserToGroup`, `DartGetGroupApplicationList`, `DartAcceptGroupApplication`, `DartRefuseGroupApplication`, `DartSetGroupApplicationRead`, `DartInitGroupAttributes`, `DartSetGroupAttributes`, `DartDeleteGroupAttributes`, `DartGetGroupAttributes`, `DartSetGroupCounters`, `DartGetGroupCounters`, `DartIncreaseGroupCounter`, `DartDecreaseGroupCounter`, `DartSearchGroupMembers`, `DartSearchCloudGroupMembers`, etc.
- ✅ Session functions: `DartGetConversationList`, `DartGetConversation`, `DartGetConversationListByFilter`, `DartDeleteConversation`, `DartDeleteConversationList`, `DartSetConversationDraft`, `DartCancelConversationDraft`, `DartSetConversationCustomData`, `DartMarkConversationAsRead`, `DartGetTotalUnreadMessageCount`, `DartGetUnreadMessageCountByFilter`, `DartCreateConversationGroup`, `DartGetConversationGroupList`, `DartDeleteConversationGroup`, `DartRenameConversationGroup`, `DartAddConversationsToGroup`, `DartDeleteConversationsFromGroup`, etc.
- ✅ User profile functions: `DartGetUsersInfo`, `DartSetSelfInfo`, `DartGetUserStatus`, `DartSetSelfStatus`, `DartSubscribeUserStatus`, `DartUnsubscribeUserStatus`, `DartSubscribeUserInfo`, `DartUnsubscribeUserInfo`, `DartSearchUsers`, etc.
- ✅ Message option functions: `DartSetC2CReceiveMessageOpt`, `DartGetC2CReceiveMessageOpt`, `DartSetGroupReceiveMessageOpt`, `DartGetGroupReceiveMessageOpt`, etc.
- ✅ Signaling functions: `DartInvite`, `DartInviteInGroup`, `DartCancel`, `DartAccept`, `DartReject`, `DartGetSignalingInfo`, `DartModifyInvitation`, etc.
- ✅ Offline push function: `DartSetOfflinePushToken`, `DartDoBackground`, `DartDoForeground`, etc.

**To be completed** (~4-10 items):
- A small number of uncommon or experimental functions

### To be completed

#### ⏳ V2TIM object to JSON conversion

The following conversion tools need to be implemented:

- `V2TIMMessage` → JSON
- `V2TIMFriendInfoVector` → JSON
- `V2TIMConversationVector` → JSON
- `V2TIMGroupTipsElem` → JSON
- `V2TIMSignalingInfo` → JSON
- `V2TIMTopicInfo` → JSON

#### ⏳ Listener complete implementation

The implementation of all Listener methods needs to be completed:

- `DartConversationListenerImpl`: All callback methods
- `DartGroupListenerImpl`: All callback methods
- `DartSignalingListenerImpl`: All callback methods
- `DartCommunityListenerImpl`: All callback methods
- Improve the remaining methods of other Listeners

#### ⏳ Function symbol export verification

Verify that all function symbols are exported correctly and ensure that the Dart layer can be found and called normally through FFI.

## User Guide

### Function implementation mode

#### Callback setting function

```cpp
void DartSetOnAddFriendCallback(void* user_data) {
    // 1. Store user_data (converted to string)
    std::string userDataStr = user_data ? 
        std::string(static_cast<const char*>(user_data)) : "";
    
    // 2. Create or obtain Listener instance
    static std::shared_ptr<DartFriendshipListenerImpl> listener = 
        std::make_shared<DartFriendshipListenerImpl>(userDataStr);
    
    // 3. Register Listener
    auto mgr = V2TIMManager::GetInstance()->GetFriendshipManager();
    mgr->AddFriendListener(listener.get());
}
```

#### Business API functions

```cpp
void DartLogin(const char* json_login_param, void* user_data) {
    // 1. Parse JSON parameters
    std::string userID = ExtractJsonValue(json_login_param, "userID");
    std::string userSig = ExtractJsonValue(json_login_param, "userSig");
    std::string userDataStr = user_data ? 
        std::string(static_cast<const char*>(user_data)) : "";
    
    // 2. Call V2TIM API
    auto mgr = V2TIMManager::GetInstance();
    mgr->Login(
        V2TIMString(userID.c_str()),
        V2TIMString(userSig.c_str()),
        new V2TIMCallback(
            [userDataStr]() {
                // successful callback
                std::string json = BuildApiCallbackJson(
                    userDataStr,
                    {{"code", "0"}, {"desc", "OK"}}
                );
                SendCallbackToDart("apiCallback", json, nullptr);
            },
            [userDataStr](int code, const V2TIMString& msg) {
                // Failure callback
                std::string json = BuildApiCallbackJson(
                    userDataStr,
                    {{"code", std::to_string(code)}, {"desc", msg.CString()}}
                );
                SendCallbackToDart("apiCallback", json, nullptr);
            }
        )
    );
}
```

### Listener implementation pattern

```cpp
class DartAdvancedMsgListenerImpl : public V2TIMAdvancedMsgListener {
private:
    std::string userData_;
    
public:
    DartAdvancedMsgListenerImpl(const std::string& userData) 
        : userData_(userData) {}
    
    void OnRecvNewMessage(const V2TIMMessage& message) override {
        // 1. Convert V2TIM object to JSON
        std::string messageJson = MessageToJson(message);
        
        // 2. Build callback JSON
        std::string json = BuildGlobalCallbackJson(
            GlobalCallbackType::ReceiveNewMessage,
            {{"json_message", messageJson}},
            userData_
        );
        
        // 3. Send to Dart layer
        SendCallbackToDart("globalCallback", json, nullptr);
    }
    
    // ...implement other callback methods
};
```

### JSON build example

#### Build Global Callback

```cpp
std::string json = BuildGlobalCallbackJson(
    GlobalCallbackType::ReceiveNewMessage,
    {
        {"json_message", messageJson},
        {"json_offlinePushInfo", pushInfoJson}
    },
    userDataStr
);
```

#### Build API Callback

```cpp
std::string json = BuildApiCallbackJson(
    userDataStr,
    {
        {"code", "0"},
        {"desc", "OK"},
        {"json_friend_info_array", friendListJson}
    }
);
```

## Debugging Tips

### Enable debug logging

Debug logs are included in `callback_bridge.cpp` and `json_parser.cpp`:

```cpp
fprintf(stdout, "[callback_bridge] SendCallbackToDart: sent %s message\n", callback_type);
fflush(stdout);
```

### Validate JSON format

Use an online JSON validation tool to verify that the generated JSON is in the correct format.

### Check function symbols

Use `nm` or `objdump` to check that the function symbols are exported correctly:```bash
nm -D libtim2tox_ffi.dylib | grep Dart
```

### Test callback

Add logs in the Dart layer to verify whether the callback is received correctly:

```dart
receivePort.listen((message) {
  print('Received callback: $message');
  _handleNativeMessage(jsonDecode(message as String));
});
```

## Issue fixed

This section records issues discovered and fixed during development and testing for reference and to avoid duplication.

### 1. Conversation ID unified processing

**Problem description**:
- Session related functions require the full `conversationID` (with `c2c_` or `group_` prefix)
- Message related functions require base ID (without prefix)
- Different functions are processed inconsistently, resulting in format errors

**Fix Solution**:
- Implement unified auxiliary functions in `dart_compat_utils.cpp`:
  - `BuildFullConversationID()`: Build complete conversationID (with prefix)
  - `ExtractBaseConversationID()`: Extract base ID (remove prefix)
- All session related functions use `BuildFullConversationID()`
- All message related functions use `ExtractBaseConversationID()`

**Fixed functions**:
- ✅ `DartPinConversation` - Use `BuildFullConversationID`
- ✅ `DartGetConversation` - Use `BuildFullConversationID`
- ✅ `DartDeleteConversation` - Use `BuildFullConversationID`
- ✅ `DartSetConversationDraft` - Use `BuildFullConversationID`
- ✅ `DartMarkConversation` - Use `BuildFullConversationID`
- ✅ `DartSendMessage` - Use `ExtractBaseConversationID`
- ✅ `DartGetHistoryMessageList` - Use `ExtractBaseConversationID`
- ✅ `DartMarkMessageAsRead` - Use `ExtractBaseConversationID`
- ✅ `DartClearHistoryMessage` - Use `ExtractBaseConversationID`

**Key code location**:
- `tim2tox/ffi/dart_compat_utils.cpp:BuildFullConversationID()`
- `tim2tox/ffi/dart_compat_utils.cpp:ExtractBaseConversationID()`

### 2. JSON serialization format issue

**Problem description**:
- `ConversationVectorToJson` directly uses prefixed `conversationID` as `conv_id`
- `V2TimConversation.fromJson` expects `conv_id` to be the ID without the prefix
- Eventually `conversationID` becomes `c2c_c2c_xxx`, causing session duplication

**Fix Solution**:
- Extract the ID without prefix as `conv_id`
- For C2C conversations, use `userID`; for group conversations, use `groupID`
- Ensure that all serialization functions use a consistent format

**Fixed functions**:
- ✅ `ConversationVectorToJson` - Repair `conv_id` format
- ✅ `MessageSearchResultToJson` - Fix field name and `conv_id` format
- ✅ `DartConversationCallback::OnSuccess` - Use unified serialization function
- ✅ `ConversationOperationResult` - Fixed field name issues (two places)

**Key code location**:
- `tim2tox/ffi/dart_compat_utils.cpp:ConversationVectorToJson()`
-`tim2tox/ffi/dart_compat_callbacks.cpp:MessageSearchResultToJson()`

### 3. PinConversation unreadCount is not initialized

**Problem description**:
- The `V2TIMConversation` object created by `PinConversation` is not initialized `unreadCount`
- results in uninitialized values (possibly large random values), displayed as 99+

**Fix Solution**:
- Get complete session information from cache, retaining all fields
- If not in cache, initialize `unreadCount = 0`

**Key code location**:
-`tim2tox/source/V2TIMConversationManagerImpl.cpp:PinConversation()`

### 4. Event callback path repair

**Problem description**:
- `OnConversationGroupCreated`, `OnConversationsAddedToGroup`, `OnConversationsDeletedFromGroup` use placeholder `"[]"` instead of actual conversation list

**Fix Solution**:
- Use `ConversationVectorToJson(conversationList)` instead of placeholder
- Ensure that all event callbacks use a unified serialization function

**Fixed callbacks**:
- ✅ `OnConversationGroupCreated` - Use `ConversationVectorToJson`
- ✅ `OnConversationsAddedToGroup` - Use `ConversationVectorToJson`
- ✅ `OnConversationsDeletedFromGroup` - Use `ConversationVectorToJson`

**Key code location**:
-`tim2tox/ffi/dart_compat_listeners.cpp`

### 5. JSON field validation

**Problem description**:
- Some JSON field names are inconsistent with what the Dart layer expects
-Field type mismatch causes parsing failure

**Fix Solution**:
- Unified field naming rules (use underscores to separate, such as `msg_search_result_item_conv_id`)
- Make sure the field type is consistent with what the Dart layer expects
- Add field validation logic

**Fixed fields**:
- ✅ `MessageSearchResultToJson` - Use correct field names
- ✅ `ConversationOperationResult` - use correct field names

## FAQ

### 1. The callback is not triggered

**Cause**: SendPort is not registered or Dart API is not initialized

**Solution**:
- Make sure `DartInitDartApiDL` and `DartRegisterSendPort` are called
- Check `IsDartPortRegistered()` return value

### 2. JSON format error

**Cause**: JSON field names do not match or are formatted incorrectly

**Solution**:
- Check that the field name matches exactly what `NativeLibraryManager` expected
- Verify format using JSON validator

### 3. Function symbol not found

**Cause**: The function is not exported correctly or is linked incorrectly

**Solution**:
- Check if the function is declared with `extern "C"`
- Check export configuration in CMakeLists.txt

### 4. Memory leak

**Cause**: The string or object was not released correctly

**Solution**:
- Use smart pointers to manage object lifecycle
- Ensure that the string sent by `Dart_PostCObject_DL` is released by the Dart layer

## Related documents

- [Modular Documentation](MODULARIZATION.en.md) - FFI layer modular structure and development guide
- [FFI Function Declaration Guide](FFI_FUNCTION_DECLARATION_GUIDE.en.md) - Function declaration specifications and checklist
- [API Reference](API_REFERENCE.en.md) - Complete API documentation
- [Tim2Tox Architecture](ARCHITECTURE.en.md) - Overall architecture design
- [Development Guide](DEVELOPMENT_GUIDE.en.md) - Development Guide