# Tim2Tox isCustomPlatform Routing Impact
> Language: [Chinese](ISCUSTOMPLATFORM_ROUTING_IMPACT.md) | [English](ISCUSTOMPLATFORM_ROUTING_IMPACT.en.md)


## Background

This modification unifies the routing logic of a total of 82 methods in 5 SDK Manager files, so that when `isCustomPlatform` is `true` (that is, Tim2ToxSdkPlatform has been registered), it is routed to the Tox layer implementation through the Platform interface instead of taking the C API path of binary replacement.

### Route change mode

```dart
// Before change (only web uses Platform):
if (kIsWeb) {
  return TencentCloudChatSdkPlatform.instance.xxx();
}
return TIM*Manager.instance.xxx();

// After the change (Tim2Tox also uses Platform):
if (kIsWeb || TencentCloudChatSdkPlatform.instance.isCustomPlatform) {
  return TencentCloudChatSdkPlatform.instance.xxx();
}
return TIM*Manager.instance.xxx();
```

---

## 1. v2_tim_manager.dart (12 methods)

### 1.1 Listener methods (3)

| Method | Tim2Tox implementation | Pre-change behavior | Post-change behavior | Impact |
|------|-------------|-----------|-----------|------|
| `setGroupListener` | Local listener list management | Go to TIMManager → C API registration callback | Go to Tim2ToxSdkPlatform local list | **Fix double registration**: TIMManager has been internally forwarded to Platform when isCustomPlatform, and the V2TIM layer registers again, resulting in repeated callbacks |
| `addGroupListener` | Same as above | Same as above | Same as above | Same as above |
| `removeGroupListener` | Same as above | Same as above | Same as above | Symmetry repair |

### 1.2 Login/Logout (2)

| Method | Tim2Tox implementation | Pre-change behavior | Post-change behavior | Impact |
|------|-------------|-----------|-----------|------|
| `login` | Call `ffiService.login()` to connect to Tox | Go to TIMManager.login → C API | Go to Tim2ToxSdkPlatform → FfiChatService.login | **Direct FFI access**: Bypass the cloud SDK login process and directly start the Tox connection. The previous path also ended up calling libtim2tox_ffi, but through an additional TIMManager wrapper layer |
| `logout` | Set `_isLoggedIn = false`, return success | Go TIMManager.logout → C API | Go Tim2ToxSdkPlatform.logout | **Simplified logout**: Tim2Tox layer directly manages status flags |

### 1.3 User status (4)

| Method | Tim2Tox implementation | Pre-change behavior | Post-change behavior | Impact |
|------|-------------|-----------|-----------|------|
| `getUserStatus` | Get online status from `ffiService.getFriendList()`, mapped to `V2TimUserStatus` | Go to C API (no Tox friend online status concept) | Go to Tim2ToxSdkPlatform → FFI friend list | **Fix online status display**: C API does not understand Tox online status, and may return the default value before; now directly obtains the online field from the Tox friend list |
| `setSelfStatus` | Stake implementation, return success | Go to C API | Go to Tim2ToxSdkPlatform (stake) | **Function unchanged**: Tox currently does not implement custom status settings, returns success but no actual operation |
| `subscribeUserStatus` | Stake implementation, return success (Tox automatic push status) | Go to C API | Go to Tim2ToxSdkPlatform (stake) | **No functional changes**: Tox protocol automatically pushes friend status changes without explicit subscription |
| `unsubscribeUserStatus` | Stake implementation, return success | Go to C API | Go to Tim2ToxSdkPlatform (stake) | **No functional changes** |

### 1.4 Group operation (3 items)

| Method | Tim2Tox implementation | Pre-change behavior | Post-change behavior | Impact |
|------|-------------|-----------|-----------|------|
| `joinGroup` | Call `ffiService.joinGroup()`, notify the listener | Go to C API | Go to Tim2ToxSdkPlatform → FFI | **Direct FFI access**: Ensure that the group addition operation is processed through Tim2Tox's FfiChatService |
| `quitGroup` | Asynchronously exit the group through `NativeLibraryManager.registerPort` + C callback | Go to C API | Go to Tim2ToxSdkPlatform → native callback | **Unified asynchronous processing**: Use Tim2Tox's callback registration mechanism |
| `dismissGroup` | Remove from `_knownGroups`, clear history, mark exited | Go to C API | Go to Tim2ToxSdkPlatform local status management | **Local status cleanup**: Tim2Tox maintains group status locally |

---

## 2. v2_tim_friendship_manager.dart (17 methods)

### 2.1 Listener methods (3)

| Method | Tim2Tox implementation | Pre-change behavior | Post-change behavior | Impact |
|------|-------------|-----------|-----------|------|
| `setFriendListener` | Local listener list management | Go to TIMFriendshipManager → C API | Go to Tim2ToxSdkPlatform local list | **Fix double registration**: Same as group listener |
| `addFriendListener` | Same as above | Same as above | Same as above | Same as above |
| `removeFriendListener` | Same as above | Same as above | Same as above | Symmetry repair |

### 2.2 Friends list query (3)

| Method | Tim2Tox implementation | Pre-change behavior | Post-change behavior | Impact |
|------|-------------|-----------|-----------|------|
| `getFriendList` | Call `ffiService.getFriendList()`, packaged as `V2TimFriendInfo` | Go C API `DartGetFriendList` | Go Tim2ToxSdkPlatform → FFI | **Fix friend list is empty**: C API's `DartGetFriendList` may return empty/incomplete data; Tim2Tox gets the complete friend list directly from the Tox layer |
| `getFriendsInfo` | Filter the specified userID from the `getFriendList()` results | Go to C API | Go to Tim2ToxSdkPlatform → FFI filter | **Fix friend information query**: Same as above, make sure to return the correct friend information |
| `setFriendInfo` | Stake implementation, return success | Go to C API | Go to Tim2ToxSdkPlatform (stake) | **Function unchanged**: Tox currently does not implement friend note modification, return success but no actual operation |

### 2.3 Friend operation (3)| Method | Tim2Tox implementation | Pre-change behavior | Post-change behavior | Impact |
|------|-------------|-----------|-----------|------|
| `addFriend` | Call `ffiService.addFriend()` and return the operation result | Go to C API | Go to Tim2ToxSdkPlatform → FFI | **Make sure to add friends through the Tox layer**: Use Tox's friend request mechanism |
| `deleteFromFriendList` | Call `ffiService.deleteFromFriendList()` | Go to C API | Go to Tim2ToxSdkPlatform → FFI | **Make sure to delete friends and go to Tox layer** |
| `checkFriend` | Filter and check friend relationships from `getFriendList()` | Go to C API | Go to Tim2ToxSdkPlatform → FFI filter | **Fix friend relationship check**: Directly judge from Tox friend list to avoid C API returning inaccurate relationship status |

### 2.4 Friend application (5)

| Method | Tim2Tox implementation | Pre-change behavior | Post-change behavior | Impact |
|------|-------------|-----------|-----------|------|
| `getFriendApplicationList` | Filter friend requests from message history | Go to C API | Go to Tim2ToxSdkPlatform → Message history filtering | **Fix friend request list**: C API cannot sense Tox friend requests; Tim2Tox extracts from message flow |
| `acceptFriendApplication` | Call `ffiService.acceptFriendRequest()` | Go to C API | Go to Tim2ToxSdkPlatform → FFI | **Make sure to accept the request and go to Tox layer** |
| `refuseFriendApplication` | Stub implementation, return success | Go to C API | Go to Tim2ToxSdkPlatform (stake) | **Tox protocol restrictions**: Tox does not have a mechanism to explicitly reject friend requests, and failure to respond is considered rejection |
| `deleteFriendApplication` | Stub implementation, return success | Go to C API | Go to Tim2ToxSdkPlatform (stake) | **Tox protocol restriction**: no explicit deletion application concept |
| `setFriendApplicationRead` | Stub implementation, return success | Go to C API | Go to Tim2ToxSdkPlatform (stake) | **Tox protocol restriction**: No application for read status concept |

### 2.5 Blacklist (3)

| Method | Tim2Tox implementation | Pre-change behavior | Post-change behavior | Impact |
|------|-------------|-----------|-----------|------|
| `addToBlackList` | Local storage of blacklists via `_prefs` (ExtendedPreferencesService) | Go to C API | Go to Tim2ToxSdkPlatform → Local preference storage | **Local Blacklist Management**: Use local persistence instead of C API |
| `deleteFromBlackList` | Same as above, removed from local storage | Go to C API | Go to Tim2ToxSdkPlatform → Local preference storage | Same as above |
| `getBlackList` | Read blacklist list from `_prefs` | Go to C API | Go to Tim2ToxSdkPlatform → Local preference storage | Same as above |

---

## 3. v2_tim_group_manager.dart (16 methods)

### 3.1 Group CRUD (4)

| Method | Tim2Tox implementation | Pre-change behavior | Post-change behavior | Impact |
|------|-------------|-----------|-----------|------|
| `createGroup` | Call `ffiService.createGroup()` | Go to C API | Go to Tim2ToxSdkPlatform → FFI | **Make sure to use the Tox layer to create a group**: Use Tox's group chat creation mechanism |
| `getJoinedGroupList` | Call `ffiService.getGroupList()`, works even if SDK is not fully initialized | Go C API `DartGetJoinedGroupList` (may return empty) | Go Tim2ToxSdkPlatform → FFI | **Fix group list is empty**: C API may return empty group list; Tim2Tox gets it directly from Tox layer |
| `getGroupsInfo` | Get the group name/avatar from `_prefs` and construct the `GroupInfo` object | Go to C API `DartGetGroupsInfo` (return empty information) | Go to Tim2ToxSdkPlatform → Local preference storage | **Fix group information is empty**: C API returns empty GroupInfo for Tox group; Tim2Tox obtains from local storage |
| `setGroupInfo` | Save group name/avatar to `_prefs` | Go to C API | Go to Tim2ToxSdkPlatform → Local preference storage | **Local persistent group information** |

### 3.2 Group members (6)

| Method | Tim2Tox implementation | Pre-change behavior | Post-change behavior | Impact |
|------|-------------|-----------|-----------|------|
| `getGroupMemberList` | Obtained through `NativeLibraryManager.DartGetGroupMemberList` | Go to C API | Go to Tim2ToxSdkPlatform → native FFI direct adjustment | **Unified member list acquisition path** |
| `getGroupMembersInfo` | Build full member information from `ffiService` and local state | Go C API | Go Tim2ToxSdkPlatform → FFI + local state | **Fix member information**: merge FFI data and local metadata |
| `setGroupMemberInfo` | Stake implementation, return success | Go to C API | Go to Tim2ToxSdkPlatform (stake) | **Tox protocol restrictions**: Tox group members have no custom information fields |
| `muteGroupMember` | Stake implementation, return success | Go to C API | Go to Tim2ToxSdkPlatform (stake) | **Tox protocol restrictions**: Tox does not support muting |
| `inviteUserToGroup` | Call `ffiService.inviteUserToGroup()` | Go to C API | Go to Tim2ToxSdkPlatform → FFI | **Make sure the invitation goes to Tox layer** |
| `kickGroupMember` | By `NativeLibraryManager.DartKickGroupMember` + callback | Go to C API | Go to Tim2ToxSdkPlatform → native callback | **Unified kicking path** |

### 3.3 Group characters (2)| Method | Tim2Tox implementation | Pre-change behavior | Post-change behavior | Impact |
|------|-------------|-----------|-----------|------|
| `setGroupMemberRole` | Stake implementation, return success | Go to C API | Go to Tim2ToxSdkPlatform (stake) | **Tox protocol restrictions**: Tox group has no role/permission concept |
| `transferGroupOwner` | Stake implementation, return success | Go to C API | Go to Tim2ToxSdkPlatform (stake) | **Tox protocol restrictions**: Tox group owner-less transfer concept |

### 3.4 Group Application (4)

| Method | Tim2Tox implementation | Pre-change behavior | Post-change behavior | Impact |
|------|-------------|-----------|-----------|------|
| `getGroupApplicationList` | Stub/minimum implementation | Go to C API | Go to Tim2ToxSdkPlatform (stake) | **Tox protocol restrictions**: Tox group has no application approval mechanism |
| `acceptGroupApplication` | Stub implementation | Go to C API | Go to Tim2ToxSdkPlatform (stake) | Same as above |
| `refuseGroupApplication` | Stub implementation | Go to C API | Go to Tim2ToxSdkPlatform (stake) | Same as above |
| `setGroupApplicationRead` | Stub implementation | Go to C API | Go to Tim2ToxSdkPlatform (stake) | Same as above |

---

## 4. v2_tim_message_manager.dart (12 methods)

### 4.1 Read status (3)

| Method | Tim2Tox implementation | Pre-change behavior | Post-change behavior | Impact |
|------|-------------|-----------|-----------|------|
| `markC2CMessageAsRead` | Traverse historical messages and call `ffiService.markMessageAsRead()` one by one | Go to C API | Go to Tim2ToxSdkPlatform → FFI to mark one by one | **Fix read receipts**: C API's read mark for Tox messages may be invalid; Tim2Tox marks directly through FFI |
| `markGroupMessageAsRead` | Same as above, for group messages | Go to C API | Go to Tim2ToxSdkPlatform → FFI | Same as above |
| `sendMessageReadReceipts` | Return success | Go to C API | Go to Tim2ToxSdkPlatform | **Simplified processing**: Tox's read receipt has been processed at markAsRead |

### 4.2 Message search and clearing (4 items)

| Method | Tim2Tox implementation | Pre-change behavior | Post-change behavior | Impact |
|------|-------------|-----------|-----------|------|
| `findMessages` | Search message ID via conversationManager | Go C API | Go Tim2ToxSdkPlatform → Session Manager | **Fix message lookup**: Search from Tim2Tox's session manager |
| `clearC2CHistoryMessage` | Call `ffiService.clearC2CHistory()`, notify UIKit | Go to C API | Go to Tim2ToxSdkPlatform → FFI + UIKit notification | **Make sure to clear the record and go to Tox layer** |
| `clearGroupHistoryMessage` | Call `ffiService.clearGroupHistory()`, notify UIKit | Go to C API | Go to Tim2ToxSdkPlatform → FFI + UIKit notification | Same as above |
| `modifyMessage` | Stub implementation, return success + changeInfo | Go to C API | Go to Tim2ToxSdkPlatform (stake) | **Tox protocol restriction**: Tox does not support message editing |

### 4.3 Message response (2)

| Method | Tim2Tox implementation | Pre-change behavior | Post-change behavior | Impact |
|------|-------------|-----------|-----------|------|
| `addMessageReaction` | Update local message reaction data, notify listener | Go to C API | Go to Tim2ToxSdkPlatform → Local data management | **Local reaction management**: Record responses in message metadata |
| `removeMessageReaction` | Same as above, remove reaction | Go to C API | Go to Tim2ToxSdkPlatform → Local data management | Same as above |

### 4.4 Downloads and URLs (3)

| Method | Tim2Tox implementation | Pre-change behavior | Post-change behavior | Impact |
|------|-------------|-----------|-----------|------|
| `setGroupReceiveMessageOpt` | Stake implementation, return success | Go to C API | Go to Tim2ToxSdkPlatform (stake) | **Tox protocol restrictions**: Tox does not support group message Do Not Disturb |
| `getMessageOnlineUrl` | Stub — "not applicable for local P2P" | Go to C API | Go to Tim2ToxSdkPlatform (stub) | **P2P architecture difference**: Tox is local P2P transmission, no cloud URL concept |
| `downloadMessage` | Entrust `ffiService` to process | Go to C API | Go to Tim2ToxSdkPlatform → FFI | **Unified file download path** |

---

## 5. v2_tim_conversation_manager.dart (25 methods)

### 5.1 Listener methods (3)

| Method | Tim2Tox implementation | Pre-change behavior | Post-change behavior | Impact |
|------|-------------|-----------|-----------|------|
| `setConversationListener` | Local listener list management | Previously adopted Platform (completely rewritten version) | Keep using Tim2ToxSdkPlatform, adding native fallback | **No functional changes** (Tim2Tox path remains unchanged), adding native fallback for non-Tim2Tox environments |
| `addConversationListener` | Same as above | Same as above | Same as above | Same as above |
| `removeConversationListener` | Same as above | Same as above | Same as above | Same as above |

### 5.2 Session query (5)| Method | Tim2Tox implementation | Pre-change behavior | Post-change behavior | Impact |
|------|-------------|-----------|-----------|------|
| `getConversationList` | Obtained via conversationManagerProvider (FakeConversationManager) | Previous Full Platform | Keep Tim2ToxSdkPlatform | **No functional changes**; Previous full rewrite fixed C2C header flashing issue |
| `getConversationListWithoutFormat` | Same as above | Same as above | Same as above | Same as above |
| `getConversation` | Get a single session from provider | Same as above | Same as above | **Key method**: This method is the core fix point of C2C head flashing bug |
| `getConversationListByConversationIds` | Get in batches from provider | Same as above | Same as above | Same as above |
| `getConversationListByFilter` | Get conditionally from provider | Same as above | Same as above | Same as above |

### 5.3 Session modification (6 items)

| Method | Tim2Tox implementation | Pre-change behavior | Post-change behavior | Impact |
|------|-------------|-----------|-----------|------|
| `pinConversation` | Top by provider | Full platform | Keep Tim2ToxSdkPlatform | **No functional changes** |
| `getTotalUnreadMessageCount` | Aggregate unreadCount from all sessions | Same as above | Same as above | Same as above |
| `deleteConversation` | Delete via provider | Same as above | Same as above | Same as above |
| `deleteConversationList` | Batch deletion through provider | Same as above | Same as above | Same as above |
| `setConversationDraft` | Processed by Platform | Same as above | Same as above | Same as above |
| `setConversationCustomData` | Processed by Platform | Same as above | Same as above | Same as above |

### 5.4 Session marking and grouping (7 items)

| Method | Tim2Tox implementation | Pre-change behavior | Post-change behavior | Impact |
|------|-------------|-----------|-----------|------|
| `markConversation` | Handled by Platform | Full Platform | Keep Tim2ToxSdkPlatform | **No functional changes** |
| `createConversationGroup` | Processed by Platform | Same as above | Same as above | Same as above |
| `getConversationGroupList` | Processed by Platform | Same as above | Same as above | Same as above |
| `deleteConversationGroup` | Processed by Platform | Same as above | Same as above | Same as above |
| `renameConversationGroup` | Processed by Platform | Same as above | Same as above | Same as above |
| `addConversationsToGroup` | Processed by Platform | Same as above | Same as above | Same as above |
| `deleteConversationsFromGroup` | Processed by Platform | Same as above | Same as above | Same as above |

### 5.5 Unread count (4)

| Method | Tim2Tox implementation | Pre-change behavior | Post-change behavior | Impact |
|------|-------------|-----------|-----------|------|
| `getUnreadMessageCountByFilter` | Handled by Platform | Full Platform | Keep Tim2ToxSdkPlatform | **No functional changes** |
| `subscribeUnreadMessageCountByFilter` | Processed by Platform | Same as above | Same as above | Same as above |
| `unsubscribeUnreadMessageCountByFilter` | Processed by Platform | Same as above | Same as above | Same as above |
| `cleanConversationUnreadMessageCount` | Processed by Platform | Same as above | Same as above | Same as above |

---

## Summary

### Impact classification statistics

| Impact type | Number of methods | Description |
|---------|--------|------|
| **Fix critical bug** | 8 | getFriendList, getFriendsInfo, checkFriend, getJoinedGroupList, getGroupsInfo, getGroupMembersInfo, getUserStatus, getFriendApplicationList |
| **Fix dual registration** | 6 | set/add/removeGroupListener, set/add/removeFriendListener |
| **Make sure to go to the Tox layer** | 14 | login, logout, addFriend, deleteFromFriendList, createGroup, joinGroup, quitGroup, dismissGroup, inviteUserToGroup, kickGroupMember, markC2CMessageAsRead, markGroupMessageAsRead, clearC2CHistoryMessage, clearGroupHistoryMessage |
| **Stub implementation (Tox protocol restriction)** | 15 | setFriendInfo, refuseFriendApplication, deleteFriendApplication, setFriendApplicationRead, setGroupMemberInfo, muteGroupMember, setGroupMemberRole, transferGroupOwner, getGroupApplicationList, acceptGroupApplication, refuseGroupApplication, setGroupApplicationRead, modifyMessage, setGroupReceiveMessageOpt, getMessageOnlineUrl |
| **Local Data Management** | 8 | addToBlackList, deleteFromBlackList, getBlackList, setGroupInfo, addMessageReaction, removeMessageReaction, setSelfStatus, downloadMessage |
| **No functional changes (Platform has been adopted)** | 25 | All conversation manager methods (the previous completely rewritten version has ensured Platform) |
| **No functional changes (stubs/subscriptions)** | 6 | subscribeUserStatus, unsubscribeUserStatus, sendMessageReadReceipts, setConversationDraft, setConversationCustomData, findMessages |

### Stub implementation caused by Tox protocol restrictions

The following functions are not supported by the Tox protocol itself and are implemented as stubs (successful return but no actual operation):1. **Modify friend’s remarks** (setFriendInfo)
2. **Reject/delete friend application** (refuseFriendApplication, deleteFriendApplication)
3. **Friend application has been read** (setFriendApplicationRead)
4. **Group member information modification** (setGroupMemberInfo)
5. **Group mute** (muteGroupMember)
6. **Group roles/permissions** (setGroupMemberRole, transferGroupOwner)
7. **Group application approval** (getGroupApplicationList, acceptGroupApplication, refuseGroupApplication, setGroupApplicationRead)
8. **Message editing** (modifyMessage)
9. **Group message do not disturb** (setGroupReceiveMessageOpt)
10. **Message Online URL** (getMessageOnlineUrl) - P2P architecture without cloud URL
11. **Custom status setting** (setSelfStatus)

### Risk Assessment

- **Low Risk**: conversation manager methods (25) - Tim2Tox path unchanged, only native fallback added
- **Low Risk**: Listener methods (6) - fix double registration, function more correctly
- **Low risk**: stub implementation methods (15) - the C API had no practical function before
- **Medium risk**: Data query methods (8) - critical bugs fixed, but data consistency needs to be verified
- **Medium risk**: Operation methods (14) - Routing changes ensure that the Tox layer is used, and the correctness of the operation needs to be verified

### Verification Checklist

1. [ ] Open C2C chat → Confirm that the head does not flash (regression test)
2. [ ] Friends list → Confirm that friend information is loaded correctly
3. [ ] Open the group chat → Confirm that the group name/avatar is displayed correctly
4. [ ] Online status → Confirm that the friend’s online/offline status is correct
5. [ ] Add/delete friends → Confirm that the operation is normal
6. [ ] Create/leave a group → Confirm that the operation is normal
7. [ ] The message has been read → Confirm that the read receipt is normal
8. [ ] Clear chat history → Confirm that the clearing operation is normal
9. [ ] Switch between sessions → Confirm no jitter/flicker
10. [ ] Check the log → Confirm that there is no `[ERROR]` or exception