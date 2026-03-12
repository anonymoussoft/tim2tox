# Tim2Tox isCustomPlatform 路由统一影响评估
> 语言 / Language: [中文](ISCUSTOMPLATFORM_ROUTING_IMPACT.md) | [English](ISCUSTOMPLATFORM_ROUTING_IMPACT.en.md)


## 背景

本次修改统一了 5 个 SDK Manager 文件中共 82 个方法的路由逻辑，使其在 `isCustomPlatform` 为 `true`（即 Tim2ToxSdkPlatform 已注册）时，通过 Platform 接口路由到 Tox 层实现，而非走二进制替换的 C API 路径。

### 路由变更模式

```dart
// 变更前（仅 web 走 Platform）：
if (kIsWeb) {
  return TencentCloudChatSdkPlatform.instance.xxx();
}
return TIM*Manager.instance.xxx();

// 变更后（Tim2Tox 也走 Platform）：
if (kIsWeb || TencentCloudChatSdkPlatform.instance.isCustomPlatform) {
  return TencentCloudChatSdkPlatform.instance.xxx();
}
return TIM*Manager.instance.xxx();
```

---

## 一、v2_tim_manager.dart（12 个方法）

### 1.1 监听器方法（3 个）

| 方法 | Tim2Tox 实现 | 变更前行为 | 变更后行为 | 影响 |
|------|-------------|-----------|-----------|------|
| `setGroupListener` | 本地监听器列表管理 | 走 TIMManager → C API 注册回调 | 走 Tim2ToxSdkPlatform 本地列表 | **修复双重注册**：TIMManager 内部已在 isCustomPlatform 时向 Platform 转发，V2TIM 层再注册一次导致回调重复 |
| `addGroupListener` | 同上 | 同上 | 同上 | 同上 |
| `removeGroupListener` | 同上 | 同上 | 同上 | 对称修复 |

### 1.2 登录/登出（2 个）

| 方法 | Tim2Tox 实现 | 变更前行为 | 变更后行为 | 影响 |
|------|-------------|-----------|-----------|------|
| `login` | 调用 `ffiService.login()` 连接 Tox | 走 TIMManager.login → C API | 走 Tim2ToxSdkPlatform → FfiChatService.login | **直接 FFI 访问**：绕过云 SDK 登录流程，直接启动 Tox 连接。之前的路径也最终调用 libtim2tox_ffi，但经过额外的 TIMManager 包装层 |
| `logout` | 设置 `_isLoggedIn = false`，返回成功 | 走 TIMManager.logout → C API | 走 Tim2ToxSdkPlatform.logout | **简化登出**：Tim2Tox 层直接管理状态标志 |

### 1.3 用户状态（4 个）

| 方法 | Tim2Tox 实现 | 变更前行为 | 变更后行为 | 影响 |
|------|-------------|-----------|-----------|------|
| `getUserStatus` | 从 `ffiService.getFriendList()` 获取在线状态，映射为 `V2TimUserStatus` | 走 C API（无 Tox 好友在线状态概念） | 走 Tim2ToxSdkPlatform → FFI 好友列表 | **修复在线状态显示**：C API 不了解 Tox 在线状态，之前可能返回默认值；现在从 Tox 好友列表直接获取 online 字段 |
| `setSelfStatus` | 桩实现，返回成功 | 走 C API | 走 Tim2ToxSdkPlatform（桩） | **功能不变**：Tox 目前未实现自定义状态设置，返回成功但无实际操作 |
| `subscribeUserStatus` | 桩实现，返回成功（Tox 自动推送状态） | 走 C API | 走 Tim2ToxSdkPlatform（桩） | **无功能变化**：Tox 协议自动推送好友状态变更，无需显式订阅 |
| `unsubscribeUserStatus` | 桩实现，返回成功 | 走 C API | 走 Tim2ToxSdkPlatform（桩） | **无功能变化** |

### 1.4 群操作（3 个）

| 方法 | Tim2Tox 实现 | 变更前行为 | 变更后行为 | 影响 |
|------|-------------|-----------|-----------|------|
| `joinGroup` | 调用 `ffiService.joinGroup()`，通知监听器 | 走 C API | 走 Tim2ToxSdkPlatform → FFI | **直接 FFI 访问**：确保加群操作通过 Tim2Tox 的 FfiChatService 处理 |
| `quitGroup` | 通过 `NativeLibraryManager.registerPort` + C 回调异步退群 | 走 C API | 走 Tim2ToxSdkPlatform → native 回调 | **统一异步处理**：使用 Tim2Tox 的回调注册机制 |
| `dismissGroup` | 从 `_knownGroups` 移除，清空历史，标记已退出 | 走 C API | 走 Tim2ToxSdkPlatform 本地状态管理 | **本地状态清理**：Tim2Tox 在本地维护群组状态 |

---

## 二、v2_tim_friendship_manager.dart（17 个方法）

### 2.1 监听器方法（3 个）

| 方法 | Tim2Tox 实现 | 变更前行为 | 变更后行为 | 影响 |
|------|-------------|-----------|-----------|------|
| `setFriendListener` | 本地监听器列表管理 | 走 TIMFriendshipManager → C API | 走 Tim2ToxSdkPlatform 本地列表 | **修复双重注册**：与群监听器同理 |
| `addFriendListener` | 同上 | 同上 | 同上 | 同上 |
| `removeFriendListener` | 同上 | 同上 | 同上 | 对称修复 |

### 2.2 好友列表查询（3 个）

| 方法 | Tim2Tox 实现 | 变更前行为 | 变更后行为 | 影响 |
|------|-------------|-----------|-----------|------|
| `getFriendList` | 调用 `ffiService.getFriendList()`，包装为 `V2TimFriendInfo` | 走 C API `DartGetFriendList` | 走 Tim2ToxSdkPlatform → FFI | **修复好友列表为空**：C API 的 `DartGetFriendList` 可能返回空/不完整数据；Tim2Tox 直接从 Tox 层获取完整好友列表 |
| `getFriendsInfo` | 从 `getFriendList()` 结果中过滤指定 userID | 走 C API | 走 Tim2ToxSdkPlatform → FFI 过滤 | **修复好友信息查询**：同上，确保返回正确的好友资料 |
| `setFriendInfo` | 桩实现，返回成功 | 走 C API | 走 Tim2ToxSdkPlatform（桩） | **功能不变**：Tox 目前未实现好友备注修改，返回成功但无实际操作 |

### 2.3 好友操作（3 个）

| 方法 | Tim2Tox 实现 | 变更前行为 | 变更后行为 | 影响 |
|------|-------------|-----------|-----------|------|
| `addFriend` | 调用 `ffiService.addFriend()`，返回操作结果 | 走 C API | 走 Tim2ToxSdkPlatform → FFI | **确保加好友走 Tox 层**：使用 Tox 的好友请求机制 |
| `deleteFromFriendList` | 调用 `ffiService.deleteFromFriendList()` | 走 C API | 走 Tim2ToxSdkPlatform → FFI | **确保删好友走 Tox 层** |
| `checkFriend` | 从 `getFriendList()` 过滤检查好友关系 | 走 C API | 走 Tim2ToxSdkPlatform → FFI 过滤 | **修复好友关系检查**：从 Tox 好友列表直接判断，避免 C API 返回不准确的关系状态 |

### 2.4 好友申请（5 个）

| 方法 | Tim2Tox 实现 | 变更前行为 | 变更后行为 | 影响 |
|------|-------------|-----------|-----------|------|
| `getFriendApplicationList` | 从消息历史中筛选好友请求 | 走 C API | 走 Tim2ToxSdkPlatform → 消息历史过滤 | **修复好友申请列表**：C API 无法感知 Tox 好友请求；Tim2Tox 从消息流中提取 |
| `acceptFriendApplication` | 调用 `ffiService.acceptFriendRequest()` | 走 C API | 走 Tim2ToxSdkPlatform → FFI | **确保接受请求走 Tox 层** |
| `refuseFriendApplication` | 桩实现，返回成功 | 走 C API | 走 Tim2ToxSdkPlatform（桩） | **Tox 协议限制**：Tox 没有显式拒绝好友请求的机制，不响应即为拒绝 |
| `deleteFriendApplication` | 桩实现，返回成功 | 走 C API | 走 Tim2ToxSdkPlatform（桩） | **Tox 协议限制**：无显式删除申请概念 |
| `setFriendApplicationRead` | 桩实现，返回成功 | 走 C API | 走 Tim2ToxSdkPlatform（桩） | **Tox 协议限制**：无申请已读状态概念 |

### 2.5 黑名单（3 个）

| 方法 | Tim2Tox 实现 | 变更前行为 | 变更后行为 | 影响 |
|------|-------------|-----------|-----------|------|
| `addToBlackList` | 通过 `_prefs`（ExtendedPreferencesService）本地存储黑名单 | 走 C API | 走 Tim2ToxSdkPlatform → 本地偏好存储 | **本地黑名单管理**：使用本地持久化而非 C API |
| `deleteFromBlackList` | 同上，从本地存储移除 | 走 C API | 走 Tim2ToxSdkPlatform → 本地偏好存储 | 同上 |
| `getBlackList` | 从 `_prefs` 读取黑名单列表 | 走 C API | 走 Tim2ToxSdkPlatform → 本地偏好存储 | 同上 |

---

## 三、v2_tim_group_manager.dart（16 个方法）

### 3.1 群 CRUD（4 个）

| 方法 | Tim2Tox 实现 | 变更前行为 | 变更后行为 | 影响 |
|------|-------------|-----------|-----------|------|
| `createGroup` | 调用 `ffiService.createGroup()` | 走 C API | 走 Tim2ToxSdkPlatform → FFI | **确保建群走 Tox 层**：使用 Tox 的群聊创建机制 |
| `getJoinedGroupList` | 调用 `ffiService.getGroupList()`，即使 SDK 未完全初始化也可工作 | 走 C API `DartGetJoinedGroupList`（可能返回空） | 走 Tim2ToxSdkPlatform → FFI | **修复群列表为空**：C API 可能返回空群列表；Tim2Tox 直接从 Tox 层获取 |
| `getGroupsInfo` | 从 `_prefs` 获取群名/头像，构造 `GroupInfo` 对象 | 走 C API `DartGetGroupsInfo`（返回空信息） | 走 Tim2ToxSdkPlatform → 本地偏好存储 | **修复群信息为空**：C API 对 Tox 群返回空 GroupInfo；Tim2Tox 从本地存储获取 |
| `setGroupInfo` | 将群名/头像保存到 `_prefs` | 走 C API | 走 Tim2ToxSdkPlatform → 本地偏好存储 | **本地持久化群信息** |

### 3.2 群成员（6 个）

| 方法 | Tim2Tox 实现 | 变更前行为 | 变更后行为 | 影响 |
|------|-------------|-----------|-----------|------|
| `getGroupMemberList` | 通过 `NativeLibraryManager.DartGetGroupMemberList` 获取 | 走 C API | 走 Tim2ToxSdkPlatform → native FFI 直调 | **统一成员列表获取路径** |
| `getGroupMembersInfo` | 从 `ffiService` 和本地状态构建完整成员信息 | 走 C API | 走 Tim2ToxSdkPlatform → FFI + 本地状态 | **修复成员信息**：合并 FFI 数据和本地元数据 |
| `setGroupMemberInfo` | 桩实现，返回成功 | 走 C API | 走 Tim2ToxSdkPlatform（桩） | **Tox 协议限制**：Tox 群成员无自定义信息字段 |
| `muteGroupMember` | 桩实现，返回成功 | 走 C API | 走 Tim2ToxSdkPlatform（桩） | **Tox 协议限制**：Tox 不支持禁言 |
| `inviteUserToGroup` | 调用 `ffiService.inviteUserToGroup()` | 走 C API | 走 Tim2ToxSdkPlatform → FFI | **确保邀请走 Tox 层** |
| `kickGroupMember` | 通过 `NativeLibraryManager.DartKickGroupMember` + 回调 | 走 C API | 走 Tim2ToxSdkPlatform → native 回调 | **统一踢人路径** |

### 3.3 群角色（2 个）

| 方法 | Tim2Tox 实现 | 变更前行为 | 变更后行为 | 影响 |
|------|-------------|-----------|-----------|------|
| `setGroupMemberRole` | 桩实现，返回成功 | 走 C API | 走 Tim2ToxSdkPlatform（桩） | **Tox 协议限制**：Tox 群组无角色/权限概念 |
| `transferGroupOwner` | 桩实现，返回成功 | 走 C API | 走 Tim2ToxSdkPlatform（桩） | **Tox 协议限制**：Tox 群组无所有者转让概念 |

### 3.4 群申请（4 个）

| 方法 | Tim2Tox 实现 | 变更前行为 | 变更后行为 | 影响 |
|------|-------------|-----------|-----------|------|
| `getGroupApplicationList` | 桩/最小实现 | 走 C API | 走 Tim2ToxSdkPlatform（桩） | **Tox 协议限制**：Tox 群组无申请审批机制 |
| `acceptGroupApplication` | 桩实现 | 走 C API | 走 Tim2ToxSdkPlatform（桩） | 同上 |
| `refuseGroupApplication` | 桩实现 | 走 C API | 走 Tim2ToxSdkPlatform（桩） | 同上 |
| `setGroupApplicationRead` | 桩实现 | 走 C API | 走 Tim2ToxSdkPlatform（桩） | 同上 |

---

## 四、v2_tim_message_manager.dart（12 个方法）

### 4.1 已读状态（3 个）

| 方法 | Tim2Tox 实现 | 变更前行为 | 变更后行为 | 影响 |
|------|-------------|-----------|-----------|------|
| `markC2CMessageAsRead` | 遍历历史消息，逐条调用 `ffiService.markMessageAsRead()` | 走 C API | 走 Tim2ToxSdkPlatform → FFI 逐条标记 | **修复已读回执**：C API 对 Tox 消息的已读标记可能无效；Tim2Tox 通过 FFI 直接标记 |
| `markGroupMessageAsRead` | 同上，针对群消息 | 走 C API | 走 Tim2ToxSdkPlatform → FFI | 同上 |
| `sendMessageReadReceipts` | 返回成功 | 走 C API | 走 Tim2ToxSdkPlatform | **简化处理**：Tox 的已读回执在 markAsRead 时已处理 |

### 4.2 消息查找与清空（4 个）

| 方法 | Tim2Tox 实现 | 变更前行为 | 变更后行为 | 影响 |
|------|-------------|-----------|-----------|------|
| `findMessages` | 通过 conversationManager 搜索消息 ID | 走 C API | 走 Tim2ToxSdkPlatform → 会话管理器 | **修复消息查找**：从 Tim2Tox 的会话管理器中搜索 |
| `clearC2CHistoryMessage` | 调用 `ffiService.clearC2CHistory()`，通知 UIKit | 走 C API | 走 Tim2ToxSdkPlatform → FFI + UIKit 通知 | **确保清空记录走 Tox 层** |
| `clearGroupHistoryMessage` | 调用 `ffiService.clearGroupHistory()`，通知 UIKit | 走 C API | 走 Tim2ToxSdkPlatform → FFI + UIKit 通知 | 同上 |
| `modifyMessage` | 桩实现，返回成功 + changeInfo | 走 C API | 走 Tim2ToxSdkPlatform（桩） | **Tox 协议限制**：Tox 不支持消息编辑 |

### 4.3 消息回应（2 个）

| 方法 | Tim2Tox 实现 | 变更前行为 | 变更后行为 | 影响 |
|------|-------------|-----------|-----------|------|
| `addMessageReaction` | 更新本地消息 reaction 数据，通知监听器 | 走 C API | 走 Tim2ToxSdkPlatform → 本地数据管理 | **本地 reaction 管理**：在消息元数据中记录回应 |
| `removeMessageReaction` | 同上，移除 reaction | 走 C API | 走 Tim2ToxSdkPlatform → 本地数据管理 | 同上 |

### 4.4 下载与 URL（3 个）

| 方法 | Tim2Tox 实现 | 变更前行为 | 变更后行为 | 影响 |
|------|-------------|-----------|-----------|------|
| `setGroupReceiveMessageOpt` | 桩实现，返回成功 | 走 C API | 走 Tim2ToxSdkPlatform（桩） | **Tox 协议限制**：Tox 不支持群消息免打扰 |
| `getMessageOnlineUrl` | 桩 — "not applicable for local P2P" | 走 C API | 走 Tim2ToxSdkPlatform（桩） | **P2P 架构差异**：Tox 为本地 P2P 传输，无云端 URL 概念 |
| `downloadMessage` | 委托 `ffiService` 处理 | 走 C API | 走 Tim2ToxSdkPlatform → FFI | **统一文件下载路径** |

---

## 五、v2_tim_conversation_manager.dart（25 个方法）

### 5.1 监听器方法（3 个）

| 方法 | Tim2Tox 实现 | 变更前行为 | 变更后行为 | 影响 |
|------|-------------|-----------|-----------|------|
| `setConversationListener` | 本地监听器列表管理 | 之前已全走 Platform（完全重写版） | 保持走 Tim2ToxSdkPlatform，增加 native 回退 | **无功能变化**（Tim2Tox 路径不变），增加了非 Tim2Tox 环境的 native 回退 |
| `addConversationListener` | 同上 | 同上 | 同上 | 同上 |
| `removeConversationListener` | 同上 | 同上 | 同上 | 同上 |

### 5.2 会话查询（5 个）

| 方法 | Tim2Tox 实现 | 变更前行为 | 变更后行为 | 影响 |
|------|-------------|-----------|-----------|------|
| `getConversationList` | 通过 conversationManagerProvider（FakeConversationManager）获取 | 之前全走 Platform | 保持走 Tim2ToxSdkPlatform | **无功能变化**；之前的完全重写已修复了 C2C 头部闪烁问题 |
| `getConversationListWithoutFormat` | 同上 | 同上 | 同上 | 同上 |
| `getConversation` | 从 provider 获取单个会话 | 同上 | 同上 | **关键方法**：此方法是 C2C 头部闪烁 bug 的核心修复点 |
| `getConversationListByConversationIds` | 从 provider 批量获取 | 同上 | 同上 | 同上 |
| `getConversationListByFilter` | 从 provider 按条件获取 | 同上 | 同上 | 同上 |

### 5.3 会话修改（6 个）

| 方法 | Tim2Tox 实现 | 变更前行为 | 变更后行为 | 影响 |
|------|-------------|-----------|-----------|------|
| `pinConversation` | 通过 provider 置顶 | 全走 Platform | 保持走 Tim2ToxSdkPlatform | **无功能变化** |
| `getTotalUnreadMessageCount` | 从所有会话聚合 unreadCount | 同上 | 同上 | 同上 |
| `deleteConversation` | 通过 provider 删除 | 同上 | 同上 | 同上 |
| `deleteConversationList` | 通过 provider 批量删除 | 同上 | 同上 | 同上 |
| `setConversationDraft` | 通过 Platform 处理 | 同上 | 同上 | 同上 |
| `setConversationCustomData` | 通过 Platform 处理 | 同上 | 同上 | 同上 |

### 5.4 会话标记与分组（7 个）

| 方法 | Tim2Tox 实现 | 变更前行为 | 变更后行为 | 影响 |
|------|-------------|-----------|-----------|------|
| `markConversation` | 通过 Platform 处理 | 全走 Platform | 保持走 Tim2ToxSdkPlatform | **无功能变化** |
| `createConversationGroup` | 通过 Platform 处理 | 同上 | 同上 | 同上 |
| `getConversationGroupList` | 通过 Platform 处理 | 同上 | 同上 | 同上 |
| `deleteConversationGroup` | 通过 Platform 处理 | 同上 | 同上 | 同上 |
| `renameConversationGroup` | 通过 Platform 处理 | 同上 | 同上 | 同上 |
| `addConversationsToGroup` | 通过 Platform 处理 | 同上 | 同上 | 同上 |
| `deleteConversationsFromGroup` | 通过 Platform 处理 | 同上 | 同上 | 同上 |

### 5.5 未读计数（4 个）

| 方法 | Tim2Tox 实现 | 变更前行为 | 变更后行为 | 影响 |
|------|-------------|-----------|-----------|------|
| `getUnreadMessageCountByFilter` | 通过 Platform 处理 | 全走 Platform | 保持走 Tim2ToxSdkPlatform | **无功能变化** |
| `subscribeUnreadMessageCountByFilter` | 通过 Platform 处理 | 同上 | 同上 | 同上 |
| `unsubscribeUnreadMessageCountByFilter` | 通过 Platform 处理 | 同上 | 同上 | 同上 |
| `cleanConversationUnreadMessageCount` | 通过 Platform 处理 | 同上 | 同上 | 同上 |

---

## 总结

### 影响分类统计

| 影响类型 | 方法数 | 说明 |
|---------|--------|------|
| **修复关键 bug** | 8 | getFriendList, getFriendsInfo, checkFriend, getJoinedGroupList, getGroupsInfo, getGroupMembersInfo, getUserStatus, getFriendApplicationList |
| **修复双重注册** | 6 | set/add/removeGroupListener, set/add/removeFriendListener |
| **确保走 Tox 层** | 14 | login, logout, addFriend, deleteFromFriendList, createGroup, joinGroup, quitGroup, dismissGroup, inviteUserToGroup, kickGroupMember, markC2CMessageAsRead, markGroupMessageAsRead, clearC2CHistoryMessage, clearGroupHistoryMessage |
| **桩实现（Tox 协议限制）** | 15 | setFriendInfo, refuseFriendApplication, deleteFriendApplication, setFriendApplicationRead, setGroupMemberInfo, muteGroupMember, setGroupMemberRole, transferGroupOwner, getGroupApplicationList, acceptGroupApplication, refuseGroupApplication, setGroupApplicationRead, modifyMessage, setGroupReceiveMessageOpt, getMessageOnlineUrl |
| **本地数据管理** | 8 | addToBlackList, deleteFromBlackList, getBlackList, setGroupInfo, addMessageReaction, removeMessageReaction, setSelfStatus, downloadMessage |
| **无功能变化（已走 Platform）** | 25 | 全部 conversation manager 方法（之前的完全重写版已确保走 Platform） |
| **无功能变化（桩/订阅）** | 6 | subscribeUserStatus, unsubscribeUserStatus, sendMessageReadReceipts, setConversationDraft, setConversationCustomData, findMessages |

### Tox 协议限制导致的桩实现

以下功能因 Tox 协议本身不支持，为桩实现（返回成功但无实际操作）：

1. **好友备注修改**（setFriendInfo）
2. **拒绝/删除好友申请**（refuseFriendApplication, deleteFriendApplication）
3. **好友申请已读**（setFriendApplicationRead）
4. **群成员信息修改**（setGroupMemberInfo）
5. **群禁言**（muteGroupMember）
6. **群角色/权限**（setGroupMemberRole, transferGroupOwner）
7. **群申请审批**（getGroupApplicationList, acceptGroupApplication, refuseGroupApplication, setGroupApplicationRead）
8. **消息编辑**（modifyMessage）
9. **群消息免打扰**（setGroupReceiveMessageOpt）
10. **消息在线 URL**（getMessageOnlineUrl）— P2P 架构无云端 URL
11. **自定义状态设置**（setSelfStatus）

### 风险评估

- **低风险**：conversation manager 方法（25 个）— Tim2Tox 路径未变，仅增加了 native 回退
- **低风险**：监听器方法（6 个）— 修复双重注册，功能更正确
- **低风险**：桩实现方法（15 个）— 之前走 C API 也无实际功能
- **中风险**：数据查询方法（8 个）— 修复了关键 bug，但需验证数据一致性
- **中风险**：操作类方法（14 个）— 路由变更确保走 Tox 层，需验证操作正确性

### 验证清单

1. [ ] 打开 C2C 聊天 → 确认头部不闪烁（回归测试）
2. [ ] 好友列表 → 确认好友信息正确加载
3. [ ] 打开群聊 → 确认群名/头像正确显示
4. [ ] 在线状态 → 确认好友在线/离线状态正确
5. [ ] 加/删好友 → 确认操作正常
6. [ ] 建群/退群 → 确认操作正常
7. [ ] 消息已读 → 确认已读回执正常
8. [ ] 清空聊天记录 → 确认清空操作正常
9. [ ] 会话间切换 → 确认无震荡/闪烁
10. [ ] 检查日志 → 确认无 `[ERROR]` 或异常
