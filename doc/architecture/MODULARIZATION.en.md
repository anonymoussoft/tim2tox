# Tim2Tox Modularization
> Language: [Chinese](MODULARIZATION.md) | [English](MODULARIZATION.en.md)


## Overview

The Dart* function compatibility layer (`dart_compat_layer`) has been fully modularized and split from the original single large file (3200+ lines) into 13 functional modules, significantly improving the maintainability and readability of the code.

## Module structure

### Infrastructure module

#### dart_compat_internal.h
- **Responsibilities**: Shared declarations and forward declarations
- **Content**:
  - All necessary header files included
  - extern declaration of global variables
  - Predeclaration of utility functions
  - Forward declaration of Listener and Callback classes
- **number of lines**: about 91 lines

#### dart_compat_utils.cpp
- **Responsibilities**: Utility functions and global variables
- **Content**:
  - Global Listener instance definition
  - Callback user_data storage (`g_callback_user_data`)
  - Tool function implementation:
    - `StoreCallbackUserData`: Store callback user_data
    - `GetCallbackUserData`: Get callback user_data
    - `UserDataToString`: Convert user_data pointer to string
    - `SafeGetV2TIMManager`: Safely obtain V2TIMManager instance
    - `CStringToString`: C string to std::string
    - `SendApiCallbackResultWithString`: Send API callback result (string version)
    - `SendApiCallbackResult`: Send API callback results
    - `ParseJsonConfig`: Parse JSON configuration
    - `SafeGetCString`: Safely obtain C strings
    - `ConversationVectorToJson`: Convert conversation vector to JSON
- **number of lines**: about 300 lines

### Listener and callback module

#### dart_compat_listeners.cpp
- **Responsibilities**: Listener implementation and callback registration function
- **Content**:
  - `DartSDKListenerImpl`: SDK event listener
  - `DartAdvancedMsgListenerImpl`: Message event listener
  - `DartConversationListenerImpl`: Session event listener
  - `DartGroupListenerImpl`: Group event listener
  - `DartFriendshipListenerImpl`: Friend event listener
  - `DartSignalingListenerImpl`: Signaling event listener
  - `DartCommunityListenerImpl`: Community event listener
  - All `DartSet*Callback` callback registration functions (66)
- **number of lines**: about 1150 lines

#### dart_compat_callbacks.cpp
- **Responsibilities**: Callback class implementation
- **Content**:
  - `DartCallback`: Basic callback class
  - `DartSendCallback`: message sending callback
  - `DartMessageVectorCallback`: message vector callback
  - `DartFriendInfoVectorCallback`: Friend information vector callback
  - `DartConversationResultCallback`: Session result callback
  - `DartStringCallback`: String callback
  - `DartGroupInfoVectorCallback`: Group information vector callback
  - `DartConversationOperationResultVectorCallback`: Session operation result vector callback
  - `DartFriendOperationResultCallback`: Friend operation result callback
  - `DartFriendOperationResultVectorCallback`: Friend operation result vector callback
  - `DartFriendInfoResultVectorCallback`: Friend information result vector callback
  - `DartMessageCompleteCallback`: message completion callback
  - Auxiliary functions: `MessageVectorToJson`, `FriendInfoVectorToJson`, `ConversationResultToJson`, `GroupInfoVectorToJson`, `FriendOperationResultToJson`, `FriendOperationResultVectorToJson`, `FriendInfoResultVectorToJson`
- **number of lines**: about 590 lines

### Function module

#### dart_compat_sdk.cpp
- **Responsibilities**: SDK initialization and authentication functions
- **Main functions**:
  - `DartInitSDK`: Initialize SDK
  - `DartUnitSDK`: Deinitialization SDK
  - `DartGetSDKVersion`: Get SDK version
  - `DartGetServerTime`: Get server time
  - `DartSetConfig`: Setup configuration
  - `DartLogin`: Login
  - `DartLogout`: Log out
  - `DartGetLoginUserID`: Get login user ID
  - `DartGetLoginStatus`: Get login status
- **Number of lines**: about 200 lines

#### dart_compat_message.cpp
- **Responsibilities**: Message related functions
- **Main functions**:
  - `DartSendMessage`: Send message
  - `DartFindMessages`: Find messages
  - `DartRevokeMessage`: Withdraw message
  - `DartModifyMessage`: Modify message
  - `DartDeleteMessages`: Delete message
  - `DartDeleteMessageFromLocalStorage`: Delete message from local storage
  - `DartClearHistoryMessage`: Clear historical messages
  - `DartSaveMessage`: Save message
  - `DartGetHistoryMessageList`: Get a list of historical messages
  - `DartGetMessageList`: Get message list
  - `DartMarkMessageAsRead`: Mark message as read
  - `DartMarkAllMessageAsRead`: Mark all messages as read
  - `DartMarkC2CMessageAsRead`: Mark C2C message as read
  - `DartMarkGroupMessageAsRead`: Mark group messages as read
  - `DartSetLocalCustomData`: Set local custom data
  - `DartDownloadElemToPath`: Download elements to path
  - `DartDownloadMergerMessage`: Download merge message
  - and other message-related functions
- **number of lines**: about 1200 lines

#### dart_compat_friendship.cpp
- **Responsibilities**: Friend related functions
- **Main functions**:
  - `DartGetFriendList`: Get friends list
  - `DartAddFriend`: Add friends
  - `DartDeleteFromFriendList`: Removed from friends list
  - `DartGetFriendsInfo`: Get friend information
  - `DartSetFriendInfo`: Set friend information
  - `DartGetFriendApplicationList`: Get friend application list
  - `DartAcceptFriendApplication`: Accept friend request
  - `DartRefuseFriendApplication`: Reject friend request
  - `DartCheckFriend`: Check friends
  - `DartAddToBlackList`: Add to blacklist
  - `DartDeleteFromBlackList`: Removed from blacklist
  - `DartGetBlackList`: Get blacklist
  - `DartSetFriendApplicationRead`: Set friend request as read
  - and other friend-related functions
- **number of lines**: about 630 lines#### dart_compat_conversation.cpp
- **Responsibilities**: Session related functions
- **Main functions**:
  - `DartGetConversationList`: Get conversation list
  - `DartGetConversation`: Get session
  - `DartDeleteConversation`: Delete conversation
  - `DartSetConversationDraft`: Set up conversation draft
  - `DartCancelConversationDraft`: Cancel conversation draft
  - `DartPinConversation`: Pinned conversation
  - `DartMarkConversation`: Mark conversation
  - `DartGetTotalUnreadMessageCount`: Get the total number of unread messages
  - `DartGetUnreadMessageCountByFilter`: Get the number of unread messages according to filter conditions
  - `DartGetConversationListByFilter`: Get conversation list by filter conditions
  - `DartSetConversationCustomData`: Set session custom data
  - `DartCreateConversationGroup`: Create conversation groups
  - `DartDeleteConversationGroup`: Delete conversation group
  - `DartRenameConversationGroup`: Rename session grouping
  - `DartGetConversationGroupList`: Get the conversation group list
  - `DartAddConversationsToGroup`: Add conversation to group
  - `DartDeleteConversationsFromGroup`: Remove conversation from group
- **number of lines**: about 400 lines

#### dart_compat_group.cpp
- **Responsibilities**: Group related functions
- **Main functions**:
  - `DartCreateGroup`: Create a group
  - `DartJoinGroup`: Join the group
  - `DartQuitGroup`: Exit the group
  - `DartDeleteGroup`: Delete group
  - `DartGetJoinedGroupList`: Get the list of joined groups
  - `DartGetGroupsInfo`: Get group information
  - `DartSetGroupInfo`: Set group information
  - `DartGetGroupMemberList`: Get the list of group members
  - `DartGetGroupMembersInfo`: Get group member information
  - `DartInviteUserToGroup`: Invite user to group
  - `DartKickGroupMember`: Kick out group members
  - `DartModifyGroupMemberInfo`: Modify group member information
  - `DartSetGroupAttributes`: Set group properties
  - `DartGetGroupAttributes`: Get group attributes
  - `DartInitGroupAttributes`: Initialize group properties
  - `DartDeleteGroupAttributes`: Delete group attributes
  - `DartSetGroupCounters`: Set group counter
  - `DartGetGroupCounters`: Get group counter
  - `DartIncreaseGroupCounter`: Increase group counter
  - `DartDecreaseGroupCounter`: Decrement group counter
  - `DartSearchGroups`: Search group
  - `DartSearchCloudGroups`: Search cloud groups
  - `DartSearchGroupMembers`: Search group members
  - `DartSearchCloudGroupMembers`: Search cloud group members
  - `DartGetOnlineMemberCount`: Get the number of online members
  - `DartMarkGroupMemberList`: Tag group member list
  - `DartGetGroupPendencyList`: Get group pending list
  - `DartHandleGroupPendency`: Process group pending items
  - `DartMarkGroupPendencyRead`: Mark group pending items as read
- **number of lines**: about 900 lines

#### dart_compat_user.cpp
- **Responsibilities**: User related functions
- **Main functions**:
  - `DartGetUsersInfo`: Get user information
  - `DartSetSelfInfo`: Set own information
  - `DartSubscribeUserInfo`: Subscribe to user information
  - `DartUnsubscribeUserInfo`: Unsubscribe user information
  - `DartGetUserStatus`: Get user status
  - `DartSetSelfStatus`: Set own status
  - `DartSetC2CReceiveMessageOpt`: Set C2C message receiving options
  - `DartGetC2CReceiveMessageOpt`: Get C2C message receiving options
  - `DartSetAllReceiveMessageOpt`: Set all receiving message options
  - `DartGetAllReceiveMessageOpt`: Get all receiving message options
  - `DartGetLoginStatus`: Get login status
  - and other user related functions
- **number of lines**: about 550 lines

#### dart_compat_signaling.cpp
- **Responsibilities**: Signaling related functions
- **Main functions**:
  - `DartInvite`: Invitation (1 to 1)
  - `DartInviteInGroup`: Group invitation
  - `DartGetSignalingInfo`: Get signaling information
  - `DartModifyInvitation`: Modify invitation
  - `DartCancel` (DartCancelInvitation): Cancel invitation
  - `DartAccept` (DartAcceptInvitation): Accept invitation
  - `DartReject` (DartRejectInvitation): Reject invitation
- **Number of lines**: about 570 lines

#### dart_compat_community.cpp
- **Responsibilities**: Community related functions
- **Status**: ⏳ To be improved (currently a placeholder file)
- **Function to be implemented**:
  - `DartCreateCommunity`: Create a community
  - `DartDeleteCommunity`: Delete community
  - `DartSetCommunityInfo`: Set community information
  - `DartGetCommunityInfoList`: Get community information list
  - `DartGetJoinedCommunityList`: Get the list of joined communities
  - `DartCreateTopicInCommunity`: Create a topic in the community
  - `DartDeleteTopicFromCommunity`: Remove topic from community
  - `DartSetTopicInfo`: Set topic information
  - `DartGetTopicInfoList`: Get topic information list
  - and other community related functions
- **number of lines**: about 15 lines (to be improved)

#### dart_compat_other.cpp
- **RESPONSIBILITIES**: Other miscellaneous functions
- **Status**: ⏳ To be improved (currently a placeholder file)
- **Function to be implemented**:
  - `DartCallExperimentalAPI`: Call experimental API
  - `DartCheckAbility`: Check ability
  - `DartSetOfflinePushToken`: Set offline push token
  - `DartDoBackground`: Enter the background
  - `DartDoForeground`: Enter the front desk
  - and other miscellaneous functions
- **number of lines**: about 15 lines (to be improved)

### Main entry file

#### dart_compat_layer.cpp
- **Responsibilities**: Main entry file, ensure all modules are linked
- **Content**: Only includes the headers for each dart_compat_*.cpp and comments; no business logic; actual Dart* implementations live in the feature modules
- **Number of lines**: 29 lines

## Module dependencies

```
dart_compat_internal.h (shared header file)
    ↑
    ├── dart_compat_utils.cpp
    ├── dart_compat_listeners.cpp
    ├── dart_compat_callbacks.cpp
    ├── dart_compat_sdk.cpp
    ├── dart_compat_message.cpp
    ├── dart_compat_friendship.cpp
    ├── dart_compat_conversation.cpp
    ├── dart_compat_group.cpp
    ├── dart_compat_user.cpp
    ├── dart_compat_signaling.cpp
    ├── dart_compat_community.cpp
    ├── dart_compat_other.cpp
    └── dart_compat_layer.cpp (main entrance)
```

## Code statistics| module | row count | status |
|------|------|------|
| dart_compat_internal.h | ~91 | ✅ Completed |
| dart_compat_utils.cpp | ~300 | ✅ Completed |
| dart_compat_listeners.cpp | ~1150 | ✅ Completed |
| dart_compat_callbacks.cpp | ~590 | ✅ Completed |
| dart_compat_sdk.cpp | ~200 | ✅ Completed |
| dart_compat_message.cpp | ~1200 | ✅ Completed |
| dart_compat_friendship.cpp | ~630 | ✅ Completed |
| dart_compat_conversation.cpp | ~400 | ✅ Completed |
| dart_compat_group.cpp | ~900 | ✅ Complete |
| dart_compat_user.cpp | ~550 | ✅ Completed |
| dart_compat_signaling.cpp | ~570 | ✅ Completed |
| dart_compat_community.cpp | ~15 | ⏳ To be improved |
| dart_compat_other.cpp | ~15 | ⏳ To be improved |
| dart_compat_layer.cpp | 29 | ✅ Complete |
| **Total** | **~6764** | **~92% Complete** |

## Modular advantages

1. **Maintainability**: Each module focuses on a specific function, making the code clearer
2. **Compilation efficiency**: To modify a single module, you only need to recompile the module
3. **Code Organization**: Related functions are grouped together for easy search and modification.
4. **Team collaboration**: Different developers can develop different modules in parallel
5. **File size**: The largest module is about 1150 lines, which is much smaller than the original 3200+ lines.
6. **Test Friendly**: Each module can be tested independently

## Development Guide

### Add new function

1. **Determine module**: Determine which module should be added based on the function function
2. **Add implementation**: Add function implementation in the corresponding module file
3. **Make sure to export**: Function must be declared in `extern "C"` block
4. **Update Document**: Update the function list of this document

### Modify existing functions

1. **Locate module**: Use grep or IDE to search to find the module where the function is located
2. **Modify implementation**: Modify in the corresponding module file
3. **Test Verification**: Make sure the modification does not affect other functions

### Add new module

If you need to add new functional modules:

1. Create new file `dart_compat_<module>.cpp`
2. Contains `dart_compat_internal.h`
3. Implement the function in the `extern "C"` block
4. Add new files in `CMakeLists.txt`
5. Update the comments of `dart_compat_layer.cpp`

## Related documents

- [Tim2Tox FFI compatibility layer](FFI_COMPAT_LAYER.en.md) - Dart* function compatibility layer detailed description
- [Tim2Tox Architecture](ARCHITECTURE.en.md) - Overall architecture design
- [Development Guide](DEVELOPMENT_GUIDE.en.md) - Development Guide
- [FFI Function Declaration Guide](FFI_FUNCTION_DECLARATION_GUIDE.en.md) - Function declaration specifications and checklist