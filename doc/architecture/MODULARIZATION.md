# Tim2Tox FFI 层模块化
> 语言 / Language: [中文](MODULARIZATION.md) | [English](MODULARIZATION.en.md)


## 概述

Dart* 函数兼容层（`dart_compat_layer`）已完全模块化，从原来的单个大文件（3200+行）拆分为13个功能模块，显著提高了代码的可维护性和可读性。

## 模块结构

### 基础架构模块

#### dart_compat_internal.h
- **职责**: 共享声明和前置声明
- **内容**:
  - 所有必要的头文件包含
  - 全局变量的 extern 声明
  - 工具函数的前置声明
  - Listener 和 Callback 类的前置声明
- **行数**: 约91行

#### dart_compat_utils.cpp
- **职责**: 工具函数和全局变量
- **内容**:
  - 全局 Listener 实例定义
  - 回调 user_data 存储（`g_callback_user_data`）
  - 工具函数实现：
    - `StoreCallbackUserData`: 存储回调 user_data
    - `GetCallbackUserData`: 获取回调 user_data
    - `UserDataToString`: 将 user_data 指针转换为字符串
    - `SafeGetV2TIMManager`: 安全获取 V2TIMManager 实例
    - `CStringToString`: C 字符串转 std::string
    - `SendApiCallbackResultWithString`: 发送 API 回调结果（字符串版本）
    - `SendApiCallbackResult`: 发送 API 回调结果
    - `ParseJsonConfig`: 解析 JSON 配置
    - `SafeGetCString`: 安全获取 C 字符串
    - `ConversationVectorToJson`: 会话向量转 JSON
- **行数**: 约300行

### 监听器和回调模块

#### dart_compat_listeners.cpp
- **职责**: 监听器实现和回调注册函数
- **内容**:
  - `DartSDKListenerImpl`: SDK 事件监听器
  - `DartAdvancedMsgListenerImpl`: 消息事件监听器
  - `DartConversationListenerImpl`: 会话事件监听器
  - `DartGroupListenerImpl`: 群组事件监听器
  - `DartFriendshipListenerImpl`: 好友事件监听器
  - `DartSignalingListenerImpl`: 信令事件监听器
  - `DartCommunityListenerImpl`: 社区事件监听器
  - 所有 `DartSet*Callback` 回调注册函数（66个）
- **行数**: 约1150行

#### dart_compat_callbacks.cpp
- **职责**: 回调类实现
- **内容**:
  - `DartCallback`: 基础回调类
  - `DartSendCallback`: 消息发送回调
  - `DartMessageVectorCallback`: 消息向量回调
  - `DartFriendInfoVectorCallback`: 好友信息向量回调
  - `DartConversationResultCallback`: 会话结果回调
  - `DartStringCallback`: 字符串回调
  - `DartGroupInfoVectorCallback`: 群组信息向量回调
  - `DartConversationOperationResultVectorCallback`: 会话操作结果向量回调
  - `DartFriendOperationResultCallback`: 好友操作结果回调
  - `DartFriendOperationResultVectorCallback`: 好友操作结果向量回调
  - `DartFriendInfoResultVectorCallback`: 好友信息结果向量回调
  - `DartMessageCompleteCallback`: 消息完成回调
  - 辅助函数：`MessageVectorToJson`, `FriendInfoVectorToJson`, `ConversationResultToJson`, `GroupInfoVectorToJson`, `FriendOperationResultToJson`, `FriendOperationResultVectorToJson`, `FriendInfoResultVectorToJson`
- **行数**: 约590行

### 功能模块

#### dart_compat_sdk.cpp
- **职责**: SDK 初始化和认证功能
- **主要函数**:
  - `DartInitSDK`: 初始化 SDK
  - `DartUnitSDK`: 反初始化 SDK
  - `DartGetSDKVersion`: 获取 SDK 版本
  - `DartGetServerTime`: 获取服务器时间
  - `DartSetConfig`: 设置配置
  - `DartLogin`: 登录
  - `DartLogout`: 登出
  - `DartGetLoginUserID`: 获取登录用户ID
  - `DartGetLoginStatus`: 获取登录状态
- **行数**: 约200行

#### dart_compat_message.cpp
- **职责**: 消息相关功能
- **主要函数**:
  - `DartSendMessage`: 发送消息
  - `DartFindMessages`: 查找消息
  - `DartRevokeMessage`: 撤回消息
  - `DartModifyMessage`: 修改消息
  - `DartDeleteMessages`: 删除消息
  - `DartDeleteMessageFromLocalStorage`: 从本地存储删除消息
  - `DartClearHistoryMessage`: 清空历史消息
  - `DartSaveMessage`: 保存消息
  - `DartGetHistoryMessageList`: 获取历史消息列表
  - `DartGetMessageList`: 获取消息列表
  - `DartMarkMessageAsRead`: 标记消息为已读
  - `DartMarkAllMessageAsRead`: 标记所有消息为已读
  - `DartMarkC2CMessageAsRead`: 标记 C2C 消息为已读
  - `DartMarkGroupMessageAsRead`: 标记群组消息为已读
  - `DartSetLocalCustomData`: 设置本地自定义数据
  - `DartDownloadElemToPath`: 下载元素到路径
  - `DartDownloadMergerMessage`: 下载合并消息
  - 以及其他消息相关函数
- **行数**: 约1200行

#### dart_compat_friendship.cpp
- **职责**: 好友相关功能
- **主要函数**:
  - `DartGetFriendList`: 获取好友列表
  - `DartAddFriend`: 添加好友
  - `DartDeleteFromFriendList`: 从好友列表删除
  - `DartGetFriendsInfo`: 获取好友信息
  - `DartSetFriendInfo`: 设置好友信息
  - `DartGetFriendApplicationList`: 获取好友申请列表
  - `DartAcceptFriendApplication`: 接受好友申请
  - `DartRefuseFriendApplication`: 拒绝好友申请
  - `DartCheckFriend`: 检查好友
  - `DartAddToBlackList`: 添加到黑名单
  - `DartDeleteFromBlackList`: 从黑名单删除
  - `DartGetBlackList`: 获取黑名单
  - `DartSetFriendApplicationRead`: 设置好友申请为已读
  - 以及其他好友相关函数
- **行数**: 约630行

#### dart_compat_conversation.cpp
- **职责**: 会话相关功能
- **主要函数**:
  - `DartGetConversationList`: 获取会话列表
  - `DartGetConversation`: 获取会话
  - `DartDeleteConversation`: 删除会话
  - `DartSetConversationDraft`: 设置会话草稿
  - `DartCancelConversationDraft`: 取消会话草稿
  - `DartPinConversation`: 置顶会话
  - `DartMarkConversation`: 标记会话
  - `DartGetTotalUnreadMessageCount`: 获取总未读消息数
  - `DartGetUnreadMessageCountByFilter`: 按过滤条件获取未读消息数
  - `DartGetConversationListByFilter`: 按过滤条件获取会话列表
  - `DartSetConversationCustomData`: 设置会话自定义数据
  - `DartCreateConversationGroup`: 创建会话分组
  - `DartDeleteConversationGroup`: 删除会话分组
  - `DartRenameConversationGroup`: 重命名会话分组
  - `DartGetConversationGroupList`: 获取会话分组列表
  - `DartAddConversationsToGroup`: 添加会话到分组
  - `DartDeleteConversationsFromGroup`: 从分组删除会话
- **行数**: 约400行

#### dart_compat_group.cpp
- **职责**: 群组相关功能
- **主要函数**:
  - `DartCreateGroup`: 创建群组
  - `DartJoinGroup`: 加入群组
  - `DartQuitGroup`: 退出群组
  - `DartDeleteGroup`: 删除群组
  - `DartGetJoinedGroupList`: 获取已加入群组列表
  - `DartGetGroupsInfo`: 获取群组信息
  - `DartSetGroupInfo`: 设置群组信息
  - `DartGetGroupMemberList`: 获取群组成员列表
  - `DartGetGroupMembersInfo`: 获取群组成员信息
  - `DartInviteUserToGroup`: 邀请用户到群组
  - `DartKickGroupMember`: 踢出群组成员
  - `DartModifyGroupMemberInfo`: 修改群组成员信息
  - `DartSetGroupAttributes`: 设置群组属性
  - `DartGetGroupAttributes`: 获取群组属性
  - `DartInitGroupAttributes`: 初始化群组属性
  - `DartDeleteGroupAttributes`: 删除群组属性
  - `DartSetGroupCounters`: 设置群组计数器
  - `DartGetGroupCounters`: 获取群组计数器
  - `DartIncreaseGroupCounter`: 增加群组计数器
  - `DartDecreaseGroupCounter`: 减少群组计数器
  - `DartSearchGroups`: 搜索群组
  - `DartSearchCloudGroups`: 搜索云端群组
  - `DartSearchGroupMembers`: 搜索群组成员
  - `DartSearchCloudGroupMembers`: 搜索云端群组成员
  - `DartGetOnlineMemberCount`: 获取在线成员数
  - `DartMarkGroupMemberList`: 标记群组成员列表
  - `DartGetGroupPendencyList`: 获取群组待处理列表
  - `DartHandleGroupPendency`: 处理群组待处理项
  - `DartMarkGroupPendencyRead`: 标记群组待处理项为已读
- **行数**: 约900行

#### dart_compat_user.cpp
- **职责**: 用户相关功能
- **主要函数**:
  - `DartGetUsersInfo`: 获取用户信息
  - `DartSetSelfInfo`: 设置自身信息
  - `DartSubscribeUserInfo`: 订阅用户信息
  - `DartUnsubscribeUserInfo`: 取消订阅用户信息
  - `DartGetUserStatus`: 获取用户状态
  - `DartSetSelfStatus`: 设置自身状态
  - `DartSetC2CReceiveMessageOpt`: 设置 C2C 接收消息选项
  - `DartGetC2CReceiveMessageOpt`: 获取 C2C 接收消息选项
  - `DartSetAllReceiveMessageOpt`: 设置所有接收消息选项
  - `DartGetAllReceiveMessageOpt`: 获取所有接收消息选项
  - `DartGetLoginStatus`: 获取登录状态
  - 以及其他用户相关函数
- **行数**: 约550行

#### dart_compat_signaling.cpp
- **职责**: 信令相关功能
- **主要函数**:
  - `DartInvite`: 邀请（1对1）
  - `DartInviteInGroup`: 群组邀请
  - `DartGetSignalingInfo`: 获取信令信息
  - `DartModifyInvitation`: 修改邀请
  - `DartCancel` (DartCancelInvitation): 取消邀请
  - `DartAccept` (DartAcceptInvitation): 接受邀请
  - `DartReject` (DartRejectInvitation): 拒绝邀请
- **行数**: 约570行

#### dart_compat_community.cpp
- **职责**: 社区相关功能
- **状态**: ⏳ 待完善（目前为占位文件）
- **待实现函数**:
  - `DartCreateCommunity`: 创建社区
  - `DartDeleteCommunity`: 删除社区
  - `DartSetCommunityInfo`: 设置社区信息
  - `DartGetCommunityInfoList`: 获取社区信息列表
  - `DartGetJoinedCommunityList`: 获取已加入社区列表
  - `DartCreateTopicInCommunity`: 在社区中创建话题
  - `DartDeleteTopicFromCommunity`: 从社区删除话题
  - `DartSetTopicInfo`: 设置话题信息
  - `DartGetTopicInfoList`: 获取话题信息列表
  - 以及其他社区相关函数
- **行数**: 约15行（待完善）

#### dart_compat_other.cpp
- **职责**: 其他杂项功能
- **状态**: ⏳ 待完善（目前为占位文件）
- **待实现函数**:
  - `DartCallExperimentalAPI`: 调用实验性 API
  - `DartCheckAbility`: 检查能力
  - `DartSetOfflinePushToken`: 设置离线推送令牌
  - `DartDoBackground`: 进入后台
  - `DartDoForeground`: 进入前台
  - 以及其他杂项函数
- **行数**: 约15行（待完善）

### 主入口文件

#### dart_compat_layer.cpp
- **职责**: 主入口文件，确保所有模块被链接
- **内容**: 仅包含各 dart_compat_*.cpp 对应头文件与说明注释，无业务逻辑；具体 Dart* 实现分散在各功能模块中
- **行数**: 29行

## 模块依赖关系

```
dart_compat_internal.h (共享头文件)
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
    └── dart_compat_layer.cpp (主入口)
```

## 代码统计

| 模块 | 行数 | 状态 |
|------|------|------|
| dart_compat_internal.h | ~91 | ✅ 完成 |
| dart_compat_utils.cpp | ~300 | ✅ 完成 |
| dart_compat_listeners.cpp | ~1150 | ✅ 完成 |
| dart_compat_callbacks.cpp | ~590 | ✅ 完成 |
| dart_compat_sdk.cpp | ~200 | ✅ 完成 |
| dart_compat_message.cpp | ~1200 | ✅ 完成 |
| dart_compat_friendship.cpp | ~630 | ✅ 完成 |
| dart_compat_conversation.cpp | ~400 | ✅ 完成 |
| dart_compat_group.cpp | ~900 | ✅ 完成 |
| dart_compat_user.cpp | ~550 | ✅ 完成 |
| dart_compat_signaling.cpp | ~570 | ✅ 完成 |
| dart_compat_community.cpp | ~15 | ⏳ 待完善 |
| dart_compat_other.cpp | ~15 | ⏳ 待完善 |
| dart_compat_layer.cpp | 29 | ✅ 完成 |
| **总计** | **~6764** | **~92% 完成** |

## 模块化优势

1. **可维护性**: 每个模块专注于特定功能，代码更清晰
2. **编译效率**: 修改单个模块只需重新编译该模块
3. **代码组织**: 相关功能集中在一起，便于查找和修改
4. **团队协作**: 不同开发者可以并行开发不同模块
5. **文件大小**: 最大模块约1150行，远小于原来的3200+行
6. **测试友好**: 每个模块可以独立测试

## 开发指南

### 添加新函数

1. **确定模块**: 根据函数功能确定应该添加到哪个模块
2. **添加实现**: 在相应模块文件中添加函数实现
3. **确保导出**: 函数必须在 `extern "C"` 块中声明
4. **更新文档**: 更新本文档的函数列表

### 修改现有函数

1. **定位模块**: 使用 grep 或 IDE 搜索找到函数所在的模块
2. **修改实现**: 在相应模块文件中修改
3. **测试验证**: 确保修改不影响其他功能

### 添加新模块

如果需要添加新功能模块：

1. 创建新文件 `dart_compat_<module>.cpp`
2. 包含 `dart_compat_internal.h`
3. 在 `extern "C"` 块中实现函数
4. 在 `CMakeLists.txt` 中添加新文件
5. 更新 `dart_compat_layer.cpp` 的注释说明

## 相关文档

- [Tim2Tox FFI 兼容层](FFI_COMPAT_LAYER.md) - Dart* 函数兼容层详细说明
- [Tim2Tox 架构](ARCHITECTURE.md) - 整体架构设计
- [开发指南](DEVELOPMENT_GUIDE.md) - 开发指南
- [FFI 函数声明指南](FFI_FUNCTION_DECLARATION_GUIDE.md) - 函数声明规范与检查清单
