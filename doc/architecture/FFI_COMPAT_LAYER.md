# Tim2Tox FFI 兼容层
> 语言 / Language: [中文](FFI_COMPAT_LAYER.md) | [English](FFI_COMPAT_LAYER.en.md)


本文档详细说明 Dart* 函数兼容层的实现，包括回调机制、JSON 消息格式和实现状态。

## 目录

- [概述](#概述)
- [架构设计](#架构设计)
- [回调机制](#回调机制)
- [JSON 消息格式](#json-消息格式)
- [实现状态](#实现状态)
- [使用指南](#使用指南)

## 概述

Dart* 函数兼容层是二进制替换方案的核心组件，它实现了与原生 IM SDK 完全兼容的 Dart* 函数接口，使得 Dart 层的 `NativeLibraryManager` 可以无缝切换到 tim2tox 后端，而无需修改任何 Dart 代码。

> 二进制替换的整体方案、配置方式与调用链路请先读 [BINARY_REPLACEMENT.md](BINARY_REPLACEMENT.md)；本文聚焦 Dart* 兼容层本身（回调/JSON/实现状态）。

### 核心思想

1. **函数签名完全匹配**: 所有 Dart* 函数的签名与 `native_imsdk_bindings_generated.dart` 中定义的完全一致
2. **按需实现**: 仅实现当前业务/测试实际用到的 Dart* 函数（具体数量可用 `nm -D libtim2tox_ffi.* | grep Dart` 统计），而非原生 SDK 绑定中定义的全部函数（约 230 个）
3. **回调桥接**: 通过 JSON 消息和 `Dart_PostCObject_DL` 将 C++ 事件发送到 Dart 层
4. **二进制替换**: 只需替换动态库文件，Dart 层代码完全不需要修改

### 优势

- ✅ **零 Dart 代码修改**: Dart 层代码完全不需要修改
- ✅ **工作量可控**: 仅实现实际使用的函数，相比实现全部原生绑定大幅减少工作量
- ✅ **设计上兼容**: 函数签名和回调格式与原生 SDK 约定一致（随 SDK 升级需做兼容性验证）
- ✅ **易于维护**: 清晰的模块划分和代码结构

## 架构设计

### 整体架构

```
Dart 层 (NativeLibraryManager)
    ↓ bindings.DartXXX(...)
    ↓ FFI 动态查找符号
C++ 层 (dart_compat_layer.cpp)
    ↓ DartXXX() 函数实现
    ↓ JSON 解析 + 参数转换
    ↓ 调用 V2TIM SDK API
V2TIM SDK (tim2tox)
    ↓ 执行实际逻辑
    ↓ 回调（Listener）
C++ 层 (Listener 实现)
    ↓ 转换为 JSON 消息
    ↓ SendCallbackToDart()
Dart 层 (ReceivePort)
    ↓ NativeLibraryManager._handleNativeMessage()
    ↓ 分发到业务代码
```

### 核心模块

#### 1. 模块化架构

Dart* 函数兼容层已拆分为多个功能模块，提高代码可维护性：

**模块结构**:
- `dart_compat_internal.h`: 共享声明和前置声明
- `dart_compat_utils.cpp`: 工具函数和全局变量（约300行）
- `dart_compat_listeners.cpp`: 监听器实现和回调注册（约1150行）
- `dart_compat_callbacks.cpp`: 回调类实现（约590行）
- `dart_compat_sdk.cpp`: SDK初始化和认证（约200行）
- `dart_compat_message.cpp`: 消息相关功能（约1200行）
- `dart_compat_friendship.cpp`: 好友相关功能（约630行）
- `dart_compat_conversation.cpp`: 会话相关功能（约400行）
- `dart_compat_group.cpp`: 群组相关功能（约900行）
- `dart_compat_user.cpp`: 用户相关功能（约550行）
- `dart_compat_signaling.cpp`: 信令相关功能（约570行）
- `dart_compat_community.cpp`: 社区相关功能（约15行，待完善）
- `dart_compat_other.cpp`: 其他杂项功能（约15行，待完善）
- `dart_compat_layer.cpp`: 主入口文件（仅包含头文件，29行）

**总代码量**: 约6764行（分布在13个模块文件中）

**主要功能**:
- 导出实际使用的 Dart* 函数（回调设置 + 业务 API，数量见下方「实现状态」）
- 实现回调设置函数（存储 user_data，注册 Listener）
- 实现业务 API 函数（解析参数，调用 V2TIM SDK）

#### 2. callback_bridge.cpp/h

实现回调桥接机制，将 C++ 事件发送到 Dart 层。

**主要功能**:
- `DartInitDartApiDL`: 初始化 Dart API
- `DartRegisterSendPort`: 注册 Dart SendPort
- `SendCallbackToDart`: 发送回调消息到 Dart 层

#### 3. json_parser.cpp/h

实现 JSON 消息构建和解析工具。

**主要功能**:
- `BuildGlobalCallbackJson`: 构建 globalCallback JSON 消息
- `BuildApiCallbackJson`: 构建 apiCallback JSON 消息
- `ParseJsonString`: 解析 JSON 字符串
- `ExtractJsonValue`: 提取 JSON 值

#### 4. Listener 实现

实现 V2TIM Listener 接口，将事件转换为 JSON 消息。

**主要 Listener**:
- `DartSDKListenerImpl`: SDK 事件监听
- `DartAdvancedMsgListenerImpl`: 消息事件监听
- `DartFriendshipListenerImpl`: 好友事件监听
- `DartConversationListenerImpl`: 会话事件监听
- `DartGroupListenerImpl`: 群组事件监听
- `DartSignalingListenerImpl`: 信令事件监听
- `DartCommunityListenerImpl`: 社区事件监听

## 回调机制

### 初始化流程

1. **Dart 层初始化 Dart API**:
```dart
final result = bindings.DartInitDartApiDL(DartApiDL.initData);
```

2. **Dart 层注册 SendPort**:
```dart
final receivePort = ReceivePort();
bindings.DartRegisterSendPort(receivePort.sendPort.nativePort);
```

3. **C++ 层存储 SendPort**（签名需与 `native_imsdk_bindings_generated.dart` 一致，参数为 int64_t）:
```cpp
void DartRegisterSendPort(int64_t send_port) {
    g_dart_port = static_cast<Dart_Port>(send_port);
}
```

### 回调发送流程

1. **C++ 层事件触发**:
```cpp
void DartAdvancedMsgListenerImpl::OnRecvNewMessage(const V2TIMMessage& message) {
    // 构建 JSON 消息
    std::string json = BuildGlobalCallbackJson(
        GlobalCallbackType::ReceiveNewMessage,
        {{"json_message", MessageToJson(message)}}
    );
    
    // 发送到 Dart 层
    SendCallbackToDart("globalCallback", json, nullptr);
}
```

2. **通过 Dart_PostCObject_DL 发送**（当前实现中 `user_data` 未写入消息，关联请求依赖 JSON 内字段）:
```cpp
void SendCallbackToDart(const char* callback_type, const std::string& json_data, void* user_data) {
    Dart_CObject cobj;
    cobj.type = Dart_CObject_kString;
    cobj.value.as_string = strdup(json_data.c_str());
    
    Dart_PostCObject_DL(g_dart_port, &cobj);
}
```

3. **Dart 层接收消息**:
```dart
receivePort.listen((message) {
    final json = jsonDecode(message as String);
    _handleNativeMessage(json);
});
```

### 回调类型

#### Global Callback

用于事件通知，格式：

```json
{
  "callback": "globalCallback",
  "callbackType": 7,
  "json_message": "{...}",
  "user_data": "..."
}
```

#### API Callback

用于 API 调用结果，格式：

```json
{
  "callback": "apiCallback",
  "user_data": "...",
  "code": 0,
  "desc": "OK",
  "json_...": "{...}"
}
```

## JSON 消息格式

### Global Callback 格式

#### 接收新消息

```json
{
  "callback": "globalCallback",
  "callbackType": 7,
  "json_message": "{\"msgID\":\"1234567890_user123\",\"timestamp\":1234567890,\"sender\":\"user123\",\"text\":\"Hello\"}",
  "user_data": ""
}
```

#### 好友添加

```json
{
  "callback": "globalCallback",
  "callbackType": 40,
  "json_friend_info_array": "[{\"userID\":\"user123\",\"nickName\":\"Alice\"}]",
  "user_data": ""
}
```

#### 群组提示

```json
{
  "callback": "globalCallback",
  "callbackType": 17,
  "json_group_tips_elem": "{\"type\":1,\"groupID\":\"group123\",\"memberList\":[{\"userID\":\"user123\"}]}",
  "user_data": ""
}
```

### API Callback 格式

#### 登录结果

```json
{
  "callback": "apiCallback",
  "user_data": "login_123",
  "code": 0,
  "desc": "OK"
}
```

#### 获取好友列表结果

```json
{
  "callback": "apiCallback",
  "user_data": "get_friend_list_456",
  "code": 0,
  "desc": "OK",
  "json_friend_info_array": "[{\"userID\":\"user123\",\"nickName\":\"Alice\"}]"
}
```

#### 发送消息结果

```json
{
  "callback": "apiCallback",
  "user_data": "send_msg_789",
  "code": 0,
  "desc": "OK",
  "json_message": "{\"msgID\":\"1234567890_user123\",\"timestamp\":1234567890}"
}
```

### 字段命名规则

JSON 字段名必须与 `NativeLibraryManager._handleGlobalCallback` 期望的完全匹配：

- `json_message`: 消息对象
- `json_friend_info_array`: 好友信息数组
- `json_group_tips_elem`: 群组提示元素
- `json_conversation_array`: 会话数组
- `json_signaling_info`: 信令信息
- `json_topic_info`: 话题信息

### 字段兼容性处理

为了兼容不同版本的 SDK 或不同的调用方式，某些函数支持多个字段名：

#### `DartGetUsersInfo` 函数

**实现位置**: `tim2tox/ffi/dart_compat_layer.cpp:5617-5656`

该函数支持两种字段名格式：

1. **标准格式**: `get_user_profile_list_param_identifier_array`
   - 用于标准的用户资料查询参数

2. **SDK格式**: `friendship_getprofilelist_param_identifier_array`
   - 用于 SDK 内部调用（如好友信息查询）

**实现逻辑**:
```cpp
// 先尝试标准格式
std::string user_id_list_str = ExtractJsonValue(json_str, "get_user_profile_list_param_identifier_array");
if (user_id_list_str.empty()) {
    // 如果为空，回退到SDK格式
    user_id_list_str = ExtractJsonValue(json_str, "friendship_getprofilelist_param_identifier_array");
}
```

这样可以确保无论使用哪种字段名，函数都能正确解析参数，提高兼容性。

## 实现状态

### 已完成（Phase 1-2）

#### ✅ 基础设施

- `DartInitDartApiDL`: 初始化 Dart API
- `DartRegisterSendPort`: 注册 Dart SendPort
- `SendCallbackToDart`: 发送回调消息
- JSON 解析和构建工具

#### ✅ 回调设置函数（66 个）

- SDK 回调（8 个）
- 消息回调（9 个）
- 会话回调（8 个）
- 群组回调（3 个）
- 好友回调（25 个）
- 信令回调（6 个）
- 社区回调（12 个）

#### ✅ Listener 实现

- ✅ `DartSDKListenerImpl`: 核心方法实现（OnConnectSuccess, OnConnectFailed, OnKickedOffline, OnUserSigExpired, OnSelfInfoUpdated, OnUserStatusChanged, OnUserInfoChanged, OnLog）
- ✅ `DartAdvancedMsgListenerImpl`: 核心方法实现（OnRecvNewMessage, OnRecvMessageModified, OnRecvMessageRevoked, OnRecvMessageReadReceipts, OnRecvMessageExtensionsChanged, OnRecvMessageExtensionsDeleted, OnRecvMessageReactionChanged, OnRecvAllMessageReceiveOptionChanged, OnRecvGroupPinnedMessageChanged）
- ✅ `DartFriendshipListenerImpl`: 核心方法实现（OnFriendListAdded, OnFriendListDeleted, OnFriendInfoChanged, OnFriendApplicationListAdded, OnFriendApplicationListDeleted, OnFriendApplicationListRead, OnBlackListAdded, OnBlackListDeleted, OnFriendGroupCreated, OnFriendGroupDeleted, OnFriendGroupNameChanged, OnFriendsAddedToGroup, OnFriendsDeletedFromGroup, OnOfficialAccountSubscribed, OnOfficialAccountUnsubscribed, OnOfficialAccountDeleted, OnOfficialAccountInfoChanged, OnMyFollowingListChanged, OnMyFollowersListChanged, OnMutualFollowersListChanged）
- ✅ `DartConversationListenerImpl`: 所有方法实现（OnConversationChanged, OnTotalUnreadMessageCountChanged, OnUnreadMessageCountChangedByFilter, OnConversationGroupCreated, OnConversationGroupDeleted, OnConversationGroupNameChanged, OnConversationsAddedToGroup, OnConversationsDeletedFromGroup）
- ✅ `DartGroupListenerImpl`: 所有方法实现（OnGroupTipsEvent, OnGroupAttributeChanged）
- ✅ `DartSignalingListenerImpl`: 所有方法实现（OnReceiveNewInvitation, OnInvitationCancelled, OnInviteeAccepted, OnInviteeRejected, OnInvitationTimeout, OnInvitationModified）
- ✅ `DartCommunityListenerImpl`: 所有方法实现（OnTopicCreated, OnTopicDeleted, OnTopicChanged, OnReceiveTopicRESTCustomData）

### 进行中（Phase 4）

#### ✅ 业务 API 函数（~165/150+，~98%）

**已完成** (~165 个核心函数):
- ✅ SDK 基础函数: `DartInitSDK`, `DartUnitSDK`, `DartGetSDKVersion`, `DartGetServerTime`, `DartSetConfig`, `DartLogin`, `DartLogout`, `DartGetLoginUserID`, `DartGetLoginStatus`, `DartCallExperimentalAPI` 等
- ✅ 消息函数: `DartSendMessage`, `DartFindMessages`, `DartRevokeMessage`, `DartModifyMessage`, `DartDeleteMessages`, `DartGetHistoryMessageList`, `DartDownloadElemToPath`, `DartDownloadMergerMessage`, `DartSetLocalCustomData`, `DartSaveMessage`, `DartDeleteMessageFromLocalStorage`, `DartClearHistoryMessage`, `DartSearchLocalMessages`, `DartSearchCloudMessages`, `DartSendMessageReadReceipts`, `DartGetMessageReadReceipts`, `DartGetGroupMessageReadMemberList`, `DartSetMessageExtensions`, `DartGetMessageExtensions`, `DartDeleteMessageExtensions`, `DartAddMessageReaction`, `DartRemoveMessageReaction`, `DartGetMessageReactions`, `DartMarkC2CMessageAsRead`, `DartMarkGroupMessageAsRead`, `DartMarkAllMessageAsRead`, `DartTranslateText`, `DartConvertVoiceToText`, `DartPinGroupMessage`, `DartGetGroupPinnedMessageList`, `DartCreateTextMessage`, `DartCreateCustomMessage`, `DartCreateImageMessage`, `DartCreateSoundMessage`, `DartCreateVideoMessage`, `DartCreateFileMessage`, `DartCreateLocationMessage`, `DartCreateFaceMessage`, `DartCreateMergerMessage`, `DartCreateForwardMessage`, `DartCreateAtSignedGroupMessage`, `DartCreateTargetedGroupMessage` 等
- ✅ 好友函数: `DartGetFriendList`, `DartAddFriend`, `DartDeleteFromFriendList`, `DartGetFriendsInfo`, `DartSetFriendInfo`, `DartCheckFriend`, `DartGetFriendApplicationList`, `DartAcceptFriendApplication`, `DartRefuseFriendApplication`, `DartDeleteFriendApplication`, `DartCreateFriendGroup`, `DartGetFriendGroups`, `DartDeleteFriendGroup`, `DartRenameFriendGroup`, `DartAddFriendsToFriendGroup`, `DartDeleteFriendsFromFriendGroup`, `DartGetBlackList`, `DartAddToBlackList`, `DartDeleteFromBlackList`, `DartSubscribeOfficialAccount`, `DartUnsubscribeOfficialAccount`, `DartGetOfficialAccountsInfo`, `DartFollowUser`, `DartUnfollowUser`, `DartGetMyFollowingList`, `DartGetMyFollowersList`, `DartGetMutualFollowersList`, `DartGetUserFollowInfo`, `DartCheckFollowType` 等
- ✅ 群组函数: `DartCreateGroup`, `DartJoinGroup`, `DartGetJoinedGroupList`, `DartQuitGroup`, `DartDismissGroup`, `DartGetGroupsInfo`, `DartSetGroupInfo`, `DartGetGroupMemberList`, `DartGetGroupOnlineMemberCount`, `DartSetGroupMemberInfo`, `DartGetGroupMembersInfo`, `DartMuteGroupMember`, `DartKickGroupMember`, `DartSetGroupMemberRole`, `DartTransferGroupOwner`, `DartInviteUserToGroup`, `DartGetGroupApplicationList`, `DartAcceptGroupApplication`, `DartRefuseGroupApplication`, `DartSetGroupApplicationRead`, `DartInitGroupAttributes`, `DartSetGroupAttributes`, `DartDeleteGroupAttributes`, `DartGetGroupAttributes`, `DartSetGroupCounters`, `DartGetGroupCounters`, `DartIncreaseGroupCounter`, `DartDecreaseGroupCounter`, `DartSearchGroupMembers`, `DartSearchCloudGroupMembers` 等
- ✅ 会话函数: `DartGetConversationList`, `DartGetConversation`, `DartGetConversationListByFilter`, `DartDeleteConversation`, `DartDeleteConversationList`, `DartSetConversationDraft`, `DartCancelConversationDraft`, `DartSetConversationCustomData`, `DartMarkConversationAsRead`, `DartGetTotalUnreadMessageCount`, `DartGetUnreadMessageCountByFilter`, `DartCreateConversationGroup`, `DartGetConversationGroupList`, `DartDeleteConversationGroup`, `DartRenameConversationGroup`, `DartAddConversationsToGroup`, `DartDeleteConversationsFromGroup` 等
- ✅ 用户资料函数: `DartGetUsersInfo`, `DartSetSelfInfo`, `DartGetUserStatus`, `DartSetSelfStatus`, `DartSubscribeUserStatus`, `DartUnsubscribeUserStatus`, `DartSubscribeUserInfo`, `DartUnsubscribeUserInfo`, `DartSearchUsers` 等
- ✅ 消息选项函数: `DartSetC2CReceiveMessageOpt`, `DartGetC2CReceiveMessageOpt`, `DartSetGroupReceiveMessageOpt`, `DartGetGroupReceiveMessageOpt` 等
- ✅ 信令函数: `DartInvite`, `DartInviteInGroup`, `DartCancel`, `DartAccept`, `DartReject`, `DartGetSignalingInfo`, `DartModifyInvitation` 等
- ✅ 离线推送函数: `DartSetOfflinePushToken`, `DartDoBackground`, `DartDoForeground` 等

**待完成** (~4-10 个):
- 少量不常用或实验性函数

### 待完成

#### ⏳ V2TIM 对象到 JSON 转换

需要实现以下转换工具：

- `V2TIMMessage` → JSON
- `V2TIMFriendInfoVector` → JSON
- `V2TIMConversationVector` → JSON
- `V2TIMGroupTipsElem` → JSON
- `V2TIMSignalingInfo` → JSON
- `V2TIMTopicInfo` → JSON

#### ⏳ Listener 补全（可选）

上文「已完成」中已列出各 Listener 已实现的方法；部分 Listener 的个别边缘方法可能仍为占位或待补全。若发现某回调未覆盖，以源码 `ffi/dart_compat_listeners.cpp` 为准。

#### ⏳ 函数符号导出验证

验证所有函数符号是否正确导出，确保 Dart 层可以通过 FFI 正常查找和调用。

## 使用指南

### 函数实现模式

#### 回调设置函数

```cpp
void DartSetOnAddFriendCallback(void* user_data) {
    // 1. 存储 user_data（转换为字符串）
    std::string userDataStr = user_data ? 
        std::string(static_cast<const char*>(user_data)) : "";
    
    // 2. 创建或获取 Listener 实例
    static std::shared_ptr<DartFriendshipListenerImpl> listener = 
        std::make_shared<DartFriendshipListenerImpl>(userDataStr);
    
    // 3. 注册 Listener
    auto mgr = V2TIMManager::GetInstance()->GetFriendshipManager();
    mgr->AddFriendListener(listener.get());
}
```

#### 业务 API 函数

```cpp
void DartLogin(const char* json_login_param, void* user_data) {
    // 1. 解析 JSON 参数
    std::string userID = ExtractJsonValue(json_login_param, "userID");
    std::string userSig = ExtractJsonValue(json_login_param, "userSig");
    std::string userDataStr = user_data ? 
        std::string(static_cast<const char*>(user_data)) : "";
    
    // 2. 调用 V2TIM API
    auto mgr = V2TIMManager::GetInstance();
    mgr->Login(
        V2TIMString(userID.c_str()),
        V2TIMString(userSig.c_str()),
        new V2TIMCallback(
            [userDataStr]() {
                // 成功回调
                std::string json = BuildApiCallbackJson(
                    userDataStr,
                    {{"code", "0"}, {"desc", "OK"}}
                );
                SendCallbackToDart("apiCallback", json, nullptr);
            },
            [userDataStr](int code, const V2TIMString& msg) {
                // 失败回调
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

### Listener 实现模式

```cpp
class DartAdvancedMsgListenerImpl : public V2TIMAdvancedMsgListener {
private:
    std::string userData_;
    
public:
    DartAdvancedMsgListenerImpl(const std::string& userData) 
        : userData_(userData) {}
    
    void OnRecvNewMessage(const V2TIMMessage& message) override {
        // 1. 转换 V2TIM 对象为 JSON
        std::string messageJson = MessageToJson(message);
        
        // 2. 构建回调 JSON
        std::string json = BuildGlobalCallbackJson(
            GlobalCallbackType::ReceiveNewMessage,
            {{"json_message", messageJson}},
            userData_
        );
        
        // 3. 发送到 Dart 层
        SendCallbackToDart("globalCallback", json, nullptr);
    }
    
    // ... 实现其他回调方法
};
```

### JSON 构建示例

#### 构建 Global Callback

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

#### 构建 API Callback

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

## 调试技巧

### 启用调试日志

在 `callback_bridge.cpp` 和 `json_parser.cpp` 中已包含调试日志：

```cpp
fprintf(stdout, "[callback_bridge] SendCallbackToDart: sent %s message\n", callback_type);
fflush(stdout);
```

### 验证 JSON 格式

使用在线 JSON 验证工具验证生成的 JSON 格式是否正确。

### 检查函数符号

使用 `nm` 或 `objdump` 检查函数符号是否正确导出：

```bash
nm -D libtim2tox_ffi.dylib | grep Dart
```

### 测试回调

在 Dart 层添加日志，验证回调是否正确接收：

```dart
receivePort.listen((message) {
  print('Received callback: $message');
  _handleNativeMessage(jsonDecode(message as String));
});
```

## 已修复问题

本节记录在开发和测试过程中发现并已修复的问题，供参考和避免重复。

### 1. Conversation ID 统一处理

**问题描述**：
- 会话相关函数需要完整的 `conversationID`（带 `c2c_` 或 `group_` 前缀）
- 消息相关函数需要 base ID（不带前缀）
- 不同函数处理方式不一致，导致格式错误

**修复方案**：
- 在 `dart_compat_utils.cpp` 中实现统一的辅助函数：
  - `BuildFullConversationID()`: 构建完整的 conversationID（带前缀）
  - `ExtractBaseConversationID()`: 提取 base ID（去掉前缀）
- 所有会话相关函数使用 `BuildFullConversationID()`
- 所有消息相关函数使用 `ExtractBaseConversationID()`

**已修复的函数**：
- ✅ `DartPinConversation` - 使用 `BuildFullConversationID`
- ✅ `DartGetConversation` - 使用 `BuildFullConversationID`
- ✅ `DartDeleteConversation` - 使用 `BuildFullConversationID`
- ✅ `DartSetConversationDraft` - 使用 `BuildFullConversationID`
- ✅ `DartMarkConversation` - 使用 `BuildFullConversationID`
- ✅ `DartSendMessage` - 使用 `ExtractBaseConversationID`
- ✅ `DartGetHistoryMessageList` - 使用 `ExtractBaseConversationID`
- ✅ `DartMarkMessageAsRead` - 使用 `ExtractBaseConversationID`
- ✅ `DartClearHistoryMessage` - 使用 `ExtractBaseConversationID`

**关键代码位置**：
- `tim2tox/ffi/dart_compat_utils.cpp:BuildFullConversationID()`
- `tim2tox/ffi/dart_compat_utils.cpp:ExtractBaseConversationID()`

### 2. JSON 序列化格式问题

**问题描述**：
- `ConversationVectorToJson` 直接使用带前缀的 `conversationID` 作为 `conv_id`
- `V2TimConversation.fromJson` 期望 `conv_id` 是不带前缀的 ID
- 导致最终 `conversationID` 变成 `c2c_c2c_xxx`，造成会话重复

**修复方案**：
- 提取不带前缀的 ID 作为 `conv_id`
- 对于 C2C 会话，使用 `userID`；对于群组会话，使用 `groupID`
- 确保所有序列化函数使用统一的格式

**已修复的函数**：
- ✅ `ConversationVectorToJson` - 修复 `conv_id` 格式
- ✅ `MessageSearchResultToJson` - 修复字段名和 `conv_id` 格式
- ✅ `DartConversationCallback::OnSuccess` - 使用统一的序列化函数
- ✅ `ConversationOperationResult` - 修复字段名问题（两处）

**关键代码位置**：
- `tim2tox/ffi/dart_compat_utils.cpp:ConversationVectorToJson()`
- `tim2tox/ffi/dart_compat_callbacks.cpp:MessageSearchResultToJson()`

### 3. PinConversation unreadCount 未初始化

**问题描述**：
- `PinConversation` 创建的 `V2TIMConversation` 对象没有初始化 `unreadCount`
- 导致未初始化值（可能是很大的随机值），显示为 99+

**修复方案**：
- 从缓存中获取完整的会话信息，保留所有字段
- 如果缓存中没有，初始化 `unreadCount = 0`

**关键代码位置**：
- `tim2tox/source/V2TIMConversationManagerImpl.cpp:PinConversation()`

### 4. 事件回调路径修复

**问题描述**：
- `OnConversationGroupCreated`、`OnConversationsAddedToGroup`、`OnConversationsDeletedFromGroup` 使用占位符 `"[]"` 而不是实际的会话列表

**修复方案**：
- 使用 `ConversationVectorToJson(conversationList)` 替代占位符
- 确保所有事件回调使用统一的序列化函数

**已修复的回调**：
- ✅ `OnConversationGroupCreated` - 使用 `ConversationVectorToJson`
- ✅ `OnConversationsAddedToGroup` - 使用 `ConversationVectorToJson`
- ✅ `OnConversationsDeletedFromGroup` - 使用 `ConversationVectorToJson`

**关键代码位置**：
- `tim2tox/ffi/dart_compat_listeners.cpp`

### 5. JSON 字段验证

**问题描述**：
- 部分 JSON 字段名与 Dart 层期望的不一致
- 字段类型不匹配导致解析失败

**修复方案**：
- 统一字段命名规则（使用下划线分隔，如 `msg_search_result_item_conv_id`）
- 确保字段类型与 Dart 层期望的一致
- 添加字段验证逻辑

**已修复的字段**：
- ✅ `MessageSearchResultToJson` - 使用正确的字段名
- ✅ `ConversationOperationResult` - 使用正确的字段名

## 常见问题

### 1. 回调不触发

**原因**: SendPort 未注册或 Dart API 未初始化

**解决**: 
- 确保 `DartInitDartApiDL` 和 `DartRegisterSendPort` 已调用
- 检查 `IsDartPortRegistered()` 返回值

### 2. JSON 格式错误

**原因**: JSON 字段名不匹配或格式不正确

**解决**:
- 检查字段名是否与 `NativeLibraryManager` 期望的完全匹配
- 使用 JSON 验证工具验证格式

### 3. 函数符号未找到

**原因**: 函数未正确导出或链接错误

**解决**:
- 检查函数是否使用 `extern "C"` 声明
- 检查 CMakeLists.txt 中的导出配置

### 4. 内存泄漏

**原因**: 字符串或对象未正确释放

**解决**:
- 使用智能指针管理对象生命周期
- 确保 `Dart_PostCObject_DL` 发送的字符串由 Dart 层释放

## 相关文档

- [模块化文档](MODULARIZATION.md) - FFI 层模块化结构和开发指南
- [FFI 函数声明指南](../development/FFI_FUNCTION_DECLARATION_GUIDE.md) - 函数声明规范与检查清单
- [API 参考](../api/API_REFERENCE.md) - 完整 API 文档
- [Tim2Tox 架构](ARCHITECTURE.md) - 整体架构设计
- [开发指南](../development/DEVELOPMENT_GUIDE.md) - 开发指南
