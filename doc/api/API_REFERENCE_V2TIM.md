# Tim2Tox API 参考 — V2TIM C++
> 语言 / Language: [中文](API_REFERENCE_V2TIM.md) | [English](API_REFERENCE_V2TIM.en.md)

本文档为 [API_REFERENCE.md](API_REFERENCE.md) 的 V2TIM C++ 部分。

## V2TIM C++ API

Tim2Tox 实现了完整的 V2TIM API，与腾讯云 IM SDK 的 V2TIM 接口完全兼容。所有 API 定义在 `include/` 目录下的头文件中。

### V2TIMManager

核心管理器，提供 SDK 初始化、登录登出、获取其他管理器等功能。

**头文件**: `include/V2TIMManager.h`

#### 初始化

```cpp
// 获取单例实例
static V2TIMManager* GetInstance();

// 初始化 SDK
virtual bool InitSDK(uint32_t sdkAppID, const V2TIMSDKConfig& config) = 0;

// 反初始化 SDK
virtual void UnInitSDK() = 0;

// 获取 SDK 版本
virtual V2TIMString GetVersion() = 0;

// 获取服务器时间
virtual int64_t GetServerTime() = 0;
```

#### 监听器管理

```cpp
// 添加 SDK 监听
virtual void AddSDKListener(V2TIMSDKListener* listener) = 0;

// 移除 SDK 监听
virtual void RemoveSDKListener(V2TIMSDKListener* listener) = 0;
```

#### 登录登出

```cpp
// 登录
virtual void Login(const V2TIMString& userID, const V2TIMString& userSig, V2TIMCallback* callback) = 0;

// 登出
virtual void Logout(V2TIMCallback* callback) = 0;

// 获取当前登录用户 ID
virtual V2TIMString GetLoginUser() = 0;
```

#### 获取管理器

```cpp
// 获取消息管理器
virtual V2TIMMessageManager* GetMessageManager() = 0;

// 获取群组管理器
virtual V2TIMGroupManager* GetGroupManager() = 0;

// 获取会话管理器
virtual V2TIMConversationManager* GetConversationManager() = 0;

// 获取好友管理器
virtual V2TIMFriendshipManager* GetFriendshipManager() = 0;

// 获取信令管理器
virtual V2TIMSignalingManager* GetSignalingManager() = 0;

// 获取社区管理器
virtual V2TIMCommunityManager* GetCommunityManager() = 0;

// 获取离线推送管理器
virtual V2TIMOfflinePushManager* GetOfflinePushManager() = 0;
```

### V2TIMMessageManager

消息管理器，提供消息创建、发送、接收、查询等功能。

**头文件**: `include/V2TIMMessageManager.h`

#### 监听器

```cpp
// 添加高级消息监听
virtual void AddAdvancedMsgListener(V2TIMAdvancedMsgListener* listener) = 0;

// 移除高级消息监听
virtual void RemoveAdvancedMsgListener(V2TIMAdvancedMsgListener* listener) = 0;
```

#### 创建消息

```cpp
// 创建文本消息
virtual V2TIMMessage CreateTextMessage(const V2TIMString& text) = 0;

// 创建自定义消息
virtual V2TIMMessage CreateCustomMessage(const V2TIMBuffer& data) = 0;
virtual V2TIMMessage CreateCustomMessage(const V2TIMBuffer& data, const V2TIMString& description, const V2TIMString& extension) = 0;

// 创建图片消息
virtual V2TIMMessage CreateImageMessage(const V2TIMString& imagePath) = 0;

// 创建语音消息
virtual V2TIMMessage CreateSoundMessage(const V2TIMString& soundPath, uint32_t duration) = 0;

// 创建视频消息
virtual V2TIMMessage CreateVideoMessage(const V2TIMString& videoFilePath, const V2TIMString& type, uint32_t duration, const V2TIMString& snapshotPath) = 0;

// 创建文件消息
virtual V2TIMMessage CreateFileMessage(const V2TIMString& filePath, const V2TIMString& fileName) = 0;

// 创建位置消息
virtual V2TIMMessage CreateLocationMessage(const V2TIMString& desc, double longitude, double latitude) = 0;

// 创建表情消息
virtual V2TIMMessage CreateFaceMessage(uint32_t index, const V2TIMBuffer& data) = 0;

// 创建合并消息
virtual V2TIMMessage CreateMergerMessage(const V2TIMMessageVector& messageList, const V2TIMString& title, const V2TIMStringVector& abstractList, const V2TIMStringVector& compatibleText) = 0;
```

#### 发送消息

```cpp
// 发送消息
virtual void SendMessage(V2TIMMessage& message, const V2TIMString& receiver, V2TIMConversationType conversationType, V2TIMMessagePriority priority, V2TIMSendCallback* callback) = 0;

// 发送消息（带云端自定义数据）
virtual void SendMessage(V2TIMMessage& message, const V2TIMString& receiver, V2TIMConversationType conversationType, V2TIMMessagePriority priority, const V2TIMBuffer& cloudCustomData, V2TIMSendCallback* callback) = 0;
```

#### 消息查询

```cpp
// 查找消息
virtual void FindMessages(const V2TIMStringVector& messageIDList, V2TIMValueCallback<V2TIMMessageVector>* callback) = 0;

// 获取历史消息
virtual void GetHistoryMessageList(const V2TIMString& conversationID, V2TIMConversationType conversationType, const V2TIMMessageGetHistoryMessageListParam& param, V2TIMValueCallback<V2TIMMessageListResult>* callback) = 0;
```

#### 消息操作

```cpp
// 撤回消息
virtual void RevokeMessage(const V2TIMMessage& message, V2TIMCallback* callback) = 0;

// 标记消息为已读
virtual void MarkMessageAsRead(const V2TIMString& conversationID, V2TIMConversationType conversationType, V2TIMCallback* callback) = 0;

// 删除消息
virtual void DeleteMessages(const V2TIMStringVector& messageIDList, V2TIMCallback* callback) = 0;
```

### V2TIMFriendshipManager

好友管理器，提供好友添加、删除、查询等功能。

**头文件**: `include/V2TIMFriendshipManager.h`

#### 监听器

```cpp
// 添加好友监听
virtual void AddFriendListener(V2TIMFriendshipListener* listener) = 0;

// 移除好友监听
virtual void RemoveFriendListener(V2TIMFriendshipListener* listener) = 0;
```

#### 好友列表

```cpp
// 获取好友列表
virtual void GetFriendList(V2TIMValueCallback<V2TIMFriendInfoVector>* callback) = 0;

// 获取指定好友资料
virtual void GetFriendsInfo(const V2TIMStringVector& userIDList, V2TIMValueCallback<V2TIMFriendInfoResultVector>* callback) = 0;

// 设置指定好友资料
virtual void SetFriendInfo(const V2TIMFriendInfo& info, V2TIMCallback* callback) = 0;
```

#### 好友操作

```cpp
// 添加好友
virtual void AddFriend(const V2TIMFriendAddApplication& application, V2TIMValueCallback<V2TIMFriendOperationResult>* callback) = 0;

// 删除好友
virtual void DeleteFromFriendList(const V2TIMStringVector& userIDList, V2TIMFriendType deleteType, V2TIMValueCallback<V2TIMFriendOperationResultVector>* callback) = 0;

// 检查好友关系
virtual void CheckFriend(const V2TIMStringVector& userIDList, V2TIMFriendType checkType, V2TIMValueCallback<V2TIMFriendCheckResultVector>* callback) = 0;
```

#### 好友申请

```cpp
// 获取好友申请列表
virtual void GetFriendApplicationList(V2TIMValueCallback<V2TIMFriendApplicationResult>* callback) = 0;

// 接受好友申请
virtual void AcceptFriendApplication(const V2TIMFriendApplication& application, V2TIMFriendResponseType responseType, V2TIMValueCallback<V2TIMFriendOperationResult>* callback) = 0;

// 拒绝好友申请
virtual void RefuseFriendApplication(const V2TIMFriendApplication& application, V2TIMValueCallback<V2TIMFriendOperationResult>* callback) = 0;
```

### V2TIMGroupManager

群组管理器，提供群组创建、加入、管理等功能。

**头文件**: `include/V2TIMGroupManager.h`

#### 监听器

```cpp
// 添加群组监听
virtual void AddGroupListener(V2TIMGroupListener* listener) = 0;

// 移除群组监听
virtual void RemoveGroupListener(V2TIMGroupListener* listener) = 0;
```

#### 群组操作

```cpp
// 创建群组
virtual void CreateGroup(const V2TIMGroupInfo& info, const V2TIMCreateGroupMemberInfoVector& memberList, V2TIMValueCallback<V2TIMString>* callback) = 0;
```

**参数说明**:
- `info.groupType`: 群组类型
  - `"group"` - 使用新 API（`tox_group_new`），支持 `chat_id` 持久化，推荐使用
  - `"conference"` - 使用旧 API（`tox_conference_new`），依赖 savedata 恢复，仅用于兼容性
  - 如果未指定，默认使用 `"group"` 类型
- `info.groupID`: 群组 ID（可选，如果不提供会自动生成）
- `info.groupName`: 群组名称
- `memberList`: 初始成员列表（可选）

**行为**:
- 如果 `groupType == "group"`: 调用 `tox_group_new`，获取并存储 `chat_id`
- 如果 `groupType == "conference"`: 调用 `tox_conference_new`，不存储 `chat_id`
- 无论哪种类型，都会存储 `groupType` 到持久化存储

```cpp
// 获取已加入的群列表
virtual void GetJoinedGroupList(V2TIMValueCallback<V2TIMGroupInfoVector>* callback) = 0;

// 获取群资料
virtual void GetGroupsInfo(const V2TIMStringVector& groupIDList, V2TIMValueCallback<V2TIMGroupInfoResultVector>* callback) = 0;
```

**返回信息**:
- `V2TIMGroupInfo.groupType`: 群组类型（"group" 或 "conference"）
- `V2TIMGroupInfo.notification`: 群公告（仅 Group 类型支持，从 `tox_group_get_topic` 获取）

```cpp
// 修改群资料
virtual void SetGroupInfo(const V2TIMGroupInfo& info, V2TIMCallback* callback) = 0;
```

#### 群成员管理

```cpp
// 获取群成员列表
virtual void GetGroupMemberList(const V2TIMString& groupID, V2TIMGroupMemberFilter filter, const V2TIMString& nextSeq, V2TIMValueCallback<V2TIMGroupMemberInfoResult>* callback) = 0;

// 获取指定群成员资料
virtual void GetGroupMembersInfo(const V2TIMString& groupID, const V2TIMStringVector& memberList, V2TIMValueCallback<V2TIMGroupMemberFullInfoVector>* callback) = 0;

// 修改指定群成员资料
virtual void SetGroupMemberInfo(const V2TIMString& groupID, const V2TIMString& userID, const V2TIMGroupMemberInfo& info, V2TIMCallback* callback) = 0;
```

### V2TIMConversationManager

会话管理器，提供会话查询、删除等功能。

**头文件**: `include/V2TIMConversationManager.h`

#### 监听器

```cpp
// 添加会话监听
virtual void AddConversationListener(V2TIMConversationListener* listener) = 0;

// 移除会话监听
virtual void RemoveConversationListener(V2TIMConversationListener* listener) = 0;
```

#### 会话操作

```cpp
// 获取会话列表
virtual void GetConversationList(const V2TIMConversationListFilter& filter, const V2TIMString& nextSeq, uint32_t count, V2TIMValueCallback<V2TIMConversationResult>* callback) = 0;

// 获取指定会话
virtual void GetConversation(const V2TIMString& conversationID, V2TIMConversationType conversationType, V2TIMValueCallback<V2TIMConversation>* callback) = 0;

// 删除会话
virtual void DeleteConversation(const V2TIMString& conversationID, V2TIMConversationType conversationType, V2TIMCallback* callback) = 0;

// 设置会话草稿
virtual void SetConversationDraft(const V2TIMString& conversationID, V2TIMConversationType conversationType, const V2TIMString& draftText, V2TIMCallback* callback) = 0;
```

### V2TIMSignalingManager

信令管理器，提供音视频通话邀请等功能。

**头文件**: `include/V2TIMSignalingManager.h`

#### 监听器

```cpp
// 添加信令监听
virtual void AddSignalingListener(V2TIMSignalingListener* listener) = 0;

// 移除信令监听
virtual void RemoveSignalingListener(V2TIMSignalingListener* listener) = 0;
```

#### 信令操作

```cpp
// 邀请（1对1）
virtual void Invite(const V2TIMString& invitee, const V2TIMString& data, bool onlineUserOnly, int timeout, V2TIMValueCallback<V2TIMString>* callback) = 0;

// 群组邀请
virtual void InviteInGroup(const V2TIMString& groupID, const V2TIMStringVector& inviteeList, const V2TIMString& data, bool onlineUserOnly, int timeout, V2TIMValueCallback<V2TIMString>* callback) = 0;

// 取消邀请
virtual void Cancel(const V2TIMString& inviteID, const V2TIMString& data, V2TIMCallback* callback) = 0;

// 接受邀请
virtual void Accept(const V2TIMString& inviteID, const V2TIMString& data, V2TIMCallback* callback) = 0;

// 拒绝邀请
virtual void Reject(const V2TIMString& inviteID, const V2TIMString& data, V2TIMCallback* callback) = 0;
```

### V2TIMCommunityManager

社区管理器，提供话题、权限组管理等功能。

**头文件**: `include/V2TIMCommunityManager.h`

#### 监听器

```cpp
// 添加社区监听
virtual void AddCommunityListener(V2TIMCommunityListener* listener) = 0;

// 移除社区监听
virtual void RemoveCommunityListener(V2TIMCommunityListener* listener) = 0;
```

#### 话题操作

```cpp
// 创建话题
virtual void CreateTopicInCommunity(const V2TIMString& groupID, const V2TIMTopicInfo& topicInfo, V2TIMValueCallback<V2TIMString>* callback) = 0;

// 删除话题
virtual void DeleteTopicFromCommunity(const V2TIMString& groupID, const V2TIMStringVector& topicIDList, V2TIMValueCallback<V2TIMTopicOperationResultVector>* callback) = 0;

// 修改话题信息
virtual void SetTopicInfo(const V2TIMTopicInfo& topicInfo, V2TIMCallback* callback) = 0;

// 获取话题列表
virtual void GetTopicInfoList(const V2TIMString& groupID, const V2TIMStringVector& topicIDList, V2TIMValueCallback<V2TIMTopicInfoResultVector>* callback) = 0;
```
