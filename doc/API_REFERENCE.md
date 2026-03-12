# Tim2Tox API 参考
> 语言 / Language: [中文](API_REFERENCE.md) | [English](API_REFERENCE.en.md)


本文档提供 Tim2Tox 的完整 API 参考，包括 C++ V2TIM API、C FFI 接口和 Dart 包 API。

## 目录

- [V2TIM C++ API](#v2tim-c-api)
- [C FFI 接口](#c-ffi-接口)
- [Dart 包 API](#dart-包-api)
- [数据类型](#数据类型)

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

## C FFI 接口

C FFI 接口提供 C 语言绑定，供 Dart FFI 调用。所有接口定义在 `ffi/tim2tox_ffi.h` 中。

### 初始化和清理

```c
// 初始化 SDK
int tim2tox_ffi_init(void);

// 反初始化 SDK
void tim2tox_ffi_uninit(void);

// 设置文件接收目录
int tim2tox_ffi_set_file_recv_dir(const char* dir_path);
```

### 登录

```c
// 登录
int tim2tox_ffi_login(const char* user_id, const char* user_sig);

// 获取当前登录用户 ID
int tim2tox_ffi_get_login_user(char* buffer, int buffer_len);
```

### 消息

```c
// 发送 C2C 文本消息
int tim2tox_ffi_send_c2c_text(const char* user_id, const char* text);

// 发送 C2C 自定义消息
int tim2tox_ffi_send_c2c_custom(const char* user_id, const unsigned char* data, int data_len);

// 发送群组自定义消息
int tim2tox_ffi_send_group_custom(const char* group_id, const unsigned char* data, int data_len);

// 轮询接收的文本消息
int tim2tox_ffi_poll_text(char* buffer, int buffer_len);

// 轮询接收的自定义消息
int tim2tox_ffi_poll_custom(unsigned char* buffer, int buffer_len);
```

### 好友

```c
// 添加好友
int tim2tox_ffi_add_friend(const char* user_id, const char* wording);

// 获取好友列表
int tim2tox_ffi_get_friend_list(char* buffer, int buffer_len);

// 获取好友申请列表
int tim2tox_ffi_get_friend_applications(char* buffer, int buffer_len);

// 接受好友申请
int tim2tox_ffi_accept_friend(const char* user_id);

// 删除好友
int tim2tox_ffi_delete_friend(const char* user_id);

// 设置输入状态
int tim2tox_ffi_set_typing(const char* user_id, int typing_on);
```

### 群组

```c
// 创建群组
// group_type: "group" (新 API) 或 "conference" (旧 API)
int tim2tox_ffi_create_group(const char* group_name, const char* group_type, char* out_group_id, int out_len);

// 加入群组（固定使用 group API）
int tim2tox_ffi_join_group(const char* group_id, const char* request_msg);

// 发送群组文本消息
int tim2tox_ffi_send_group_text(const char* group_id, const char* text);

// 存储群组类型
int tim2tox_ffi_set_group_type(const char* group_id, const char* group_type);

// 获取群组类型
int tim2tox_ffi_get_group_type_from_storage(const char* group_id, char* out_group_type, int out_len);
```

### 文件传输

```c
// 发送文件
int tim2tox_ffi_send_file(const char* user_id, const char* file_path);

// 控制文件传输
int tim2tox_ffi_file_control(const char* user_id, uint32_t file_number, int control);
```

### 信令（音视频通话）

```c
// 添加信令监听
int tim2tox_ffi_signaling_add_listener(
    tim2tox_signaling_invitation_callback_t on_invitation,
    tim2tox_signaling_cancel_callback_t on_cancel,
    tim2tox_signaling_accept_callback_t on_accept,
    tim2tox_signaling_reject_callback_t on_reject,
    tim2tox_signaling_timeout_callback_t on_timeout,
    void* user_data
);

// 邀请用户（1对1）
int tim2tox_ffi_signaling_invite(const char* invitee, const char* data, int online_user_only, int timeout, char* out_invite_id, int out_invite_id_len);

// 群组邀请
int tim2tox_ffi_signaling_invite_in_group(const char* group_id, const char* invitee_list, const char* data, int online_user_only, int timeout, char* out_invite_id, int out_invite_id_len);

// 取消邀请
int tim2tox_ffi_signaling_cancel(const char* invite_id, const char* data);

// 接受邀请
int tim2tox_ffi_signaling_accept(const char* invite_id, const char* data);

// 拒绝邀请
int tim2tox_ffi_signaling_reject(const char* invite_id, const char* data);
```

### 音视频（ToxAV）

```c
// 初始化 ToxAV
int tim2tox_ffi_av_initialize(int64_t instance_id);

// 关闭 ToxAV
void tim2tox_ffi_av_shutdown(int64_t instance_id);

// 迭代 ToxAV（需要在主循环中定期调用）
void tim2tox_ffi_av_iterate(int64_t instance_id);

// 发起通话
int tim2tox_ffi_av_start_call(int64_t instance_id, uint32_t friend_number, uint32_t audio_bit_rate, uint32_t video_bit_rate);

// 接听通话
int tim2tox_ffi_av_answer_call(int64_t instance_id, uint32_t friend_number, uint32_t audio_bit_rate, uint32_t video_bit_rate);

// 结束通话
int tim2tox_ffi_av_end_call(int64_t instance_id, uint32_t friend_number);

// 静音/取消静音音频
int tim2tox_ffi_av_mute_audio(int64_t instance_id, uint32_t friend_number, int mute);

// 隐藏/显示视频
int tim2tox_ffi_av_mute_video(int64_t instance_id, uint32_t friend_number, int hide);

// 发送音频帧
int tim2tox_ffi_av_send_audio_frame(int64_t instance_id, uint32_t friend_number, const int16_t* pcm, size_t sample_count, uint8_t channels, uint32_t sampling_rate);

// 发送视频帧（YUV420格式）
int tim2tox_ffi_av_send_video_frame(int64_t instance_id, uint32_t friend_number, uint16_t width, uint16_t height, const uint8_t* y, const uint8_t* u, const uint8_t* v, int32_t y_stride, int32_t u_stride, int32_t v_stride);

// 设置音视频码率
int tim2tox_ffi_av_set_audio_bit_rate(int64_t instance_id, uint32_t friend_number, uint32_t audio_bit_rate);
int tim2tox_ffi_av_set_video_bit_rate(int64_t instance_id, uint32_t friend_number, uint32_t video_bit_rate);

// 注册 ToxAV 回调
void tim2tox_ffi_av_set_call_callback(int64_t instance_id, tim2tox_av_call_callback_t callback, void* user_data);
void tim2tox_ffi_av_set_call_state_callback(int64_t instance_id, tim2tox_av_call_state_callback_t callback, void* user_data);
void tim2tox_ffi_av_set_audio_receive_callback(int64_t instance_id, tim2tox_av_audio_receive_callback_t callback, void* user_data);
void tim2tox_ffi_av_set_video_receive_callback(int64_t instance_id, tim2tox_av_video_receive_callback_t callback, void* user_data);
```

### IRC 通道桥接

```c
// 连接到 IRC 服务器并加入频道
int tim2tox_ffi_irc_connect_channel(const char* server, int port, const char* channel, const char* password, const char* group_id, const char* sasl_username, const char* sasl_password, int use_ssl, const char* custom_nickname);

// 断开 IRC 频道连接
int tim2tox_ffi_irc_disconnect_channel(const char* channel);

// 发送消息到 IRC 频道
int tim2tox_ffi_irc_send_message(const char* channel, const char* message);

// 检查 IRC 频道是否已连接
int tim2tox_ffi_irc_is_connected(const char* channel);
```

### 回调机制

```c
// 事件回调类型
typedef void (*tim2tox_event_cb)(int event_type, const char* sender, const unsigned char* payload, int payload_len, void* user_data);

// 注册事件回调
void tim2tox_ffi_set_callback(tim2tox_event_cb cb, void* user_data);
```

## Dart 包 API

Dart 包提供高级 API，封装了 FFI 调用，提供更易用的接口。

### Tim2ToxFfi

底层 FFI 绑定类，直接映射到 C FFI 函数。

**文件**: `dart/lib/ffi/tim2tox_ffi.dart`

```dart
// 打开 FFI 库
static Tim2ToxFfi open()

// 设置统一日志文件
void setLogFile(String path)

// 获取当前实例 ID
int getCurrentInstanceId()

// 初始化
int init()

// 登录
int login(String userID, String userSig)

// 轮询 native 事件
int pollText(int instanceId, List<int> buffer)

// 获取登录用户
int getLoginUser(List<int> buffer)

// ToxAV
int avInitialize(int instanceId)
void avShutdown(int instanceId)
void avIterate(int instanceId)
```

### FfiChatService

高级服务层，管理消息历史、轮询、状态等。

**文件**: `dart/lib/service/ffi_chat_service.dart`

```dart
class FfiChatService {
  // 构造函数
  FfiChatService({
    ExtendedPreferencesService? preferencesService,
    LoggerService? loggerService,
    BootstrapService? bootstrapService,
    MessageHistoryPersistence? messageHistoryPersistence,
    OfflineMessageQueuePersistence? offlineMessageQueuePersistence,
    String? historyDirectory,
    String? queueFilePath,
    String? fileRecvPath,
    String? avatarsPath,
  })

  // 初始化
  Future<void> init({String? profileDirectory})

  // 登录
  Future<void> login({required String userId, required String userSig})

  // 启动轮询
  Future<void> startPolling()

  // 发送文本消息
  Future<String?> sendMessage(String peerId, String text)

  // 消息流
  Stream<ChatMessage> get messages

  // 连接状态流
  Stream<bool> get connectionStatusStream

  // 获取消息历史
  List<ChatMessage> getHistory(String id)

  // 设置输入状态
  void setTyping(String userID, bool typing)
}
```

### Tim2ToxSdkPlatform

实现 `TencentCloudChatSdkPlatform` 接口，将 UIKit SDK 调用路由到 tim2tox。

**文件**: `dart/lib/sdk/tim2tox_sdk_platform.dart`

```dart
class Tim2ToxSdkPlatform extends TencentCloudChatSdkPlatform {
  // 构造函数
  Tim2ToxSdkPlatform({
    required FfiChatService ffiService,
    EventBusProvider? eventBusProvider,
    ConversationManagerProvider? conversationManagerProvider,
    ExtendedPreferencesService? preferencesService,
  })

  // SDK 初始化
  @override
  Future<bool> initSDK({...})

  // 登录
  @override
  Future<V2TimCallback> login({...})

  // 登出
  @override
  Future<V2TimCallback> logout()

  // 发送消息
  @override
  Future<V2TimValueCallback<V2TimMessage>> sendMessage({...})

  // 获取好友列表
  @override
  Future<V2TimValueCallback<List<V2TimFriendInfo>>> getFriendList()

  // 添加好友
  @override
  Future<V2TimValueCallback<V2TimFriendOperationResult>> addFriend({...})
}
```

### 接口定义

Tim2Tox 通过接口注入客户端依赖，所有接口定义在 `dart/lib/interfaces/` 目录下。

#### ExtendedPreferencesService

偏好设置服务接口。

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

日志服务接口。

```dart
abstract class LoggerService {
  void log(String message);
  void logError(String message, Object error, StackTrace stack);
  void logWarning(String message);
  void logDebug(String message);
}
```

#### BootstrapService

Bootstrap 节点服务接口。

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

事件总线提供者接口。

```dart
abstract class EventBusProvider {
  EventBus get eventBus;
}
```

#### ConversationManagerProvider

会话管理器提供者接口。

```dart
abstract class ConversationManagerProvider {
  Future<List<FakeConversation>> getConversationList();
  Future<void> setPinned(String conversationID, bool isPinned);
  Future<void> deleteConversation(String conversationID);
  Future<int> getTotalUnreadCount();
}
```

## 数据类型

### V2TIMString

字符串类型，兼容 C++ 和 Dart。

```cpp
class V2TIMString {
    const char* CString() const;
    // ...
};
```

### V2TIMBuffer

二进制数据缓冲区。

```cpp
class V2TIMBuffer {
    const uint8_t* Data() const;
    size_t Size() const;
    // ...
};
```

### V2TIMMessage

消息对象。

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

好友信息。

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

群组信息。

```cpp
struct V2TIMGroupInfo {
    V2TIMString groupID;
    V2TIMString groupName;
    V2TIMString groupType;      // "group" (新 API) 或 "conference" (旧 API)
    V2TIMString notification;   // 群公告（仅 Group 类型支持，对应 tox_group_topic）
    V2TIMString introduction;
    // ...
};
```

**V2TIMGroupMemberFullInfo**:
```cpp
struct V2TIMGroupMemberFullInfo {
    V2TIMString userID;
    V2TIMString nickName;       // 用户昵称（从好友信息获取）
    V2TIMString nameCard;       // 群昵称（从 tox_group_peer_get_name 获取，仅 Group 类型支持）
    uint32_t role;              // 角色：OWNER, ADMIN, MEMBER
    // ...
};
```

## 回调类型

### V2TIMCallback

通用回调接口。

```cpp
class V2TIMCallback {
    virtual void OnSuccess() = 0;
    virtual void OnError(int error_code, const V2TIMString& error_message) = 0;
};
```

### V2TIMValueCallback

带返回值的回调接口。

```cpp
template<class T>
class V2TIMValueCallback {
    virtual void OnSuccess(const T& value) = 0;
    virtual void OnError(int error_code, const V2TIMString& error_message) = 0;
};
```

### V2TIMSendCallback

消息发送回调接口。

```cpp
class V2TIMSendCallback {
    virtual void OnSuccess(const V2TIMMessage& message) = 0;
    virtual void OnError(int error_code, const V2TIMString& error_message) = 0;
    virtual void OnProgress(uint32_t progress) = 0;
};
```

## 错误码

常见错误码定义在 `include/V2TIMErrorCode.h` 中：

- `ERR_SUCC` (0): 成功
- `ERR_INVALID_PARAMETERS` (6017): 无效参数
- `ERR_SDK_NOT_INITIALIZED` (6018): SDK 未初始化
- `ERR_USER_SIG_EXPIRED` (6206): 用户签名过期
- `ERR_SDK_COMM_API_CALL_FREQUENCY_LIMIT` (7008): API 调用频率限制

## 使用示例

### C++ 使用示例

```cpp
#include <V2TIMManager.h>

int main() {
    // 初始化 SDK
    V2TIMSDKConfig config;
    config.logLevel = V2TIM_LOG_DEBUG;
    V2TIMManager::GetInstance()->InitSDK(123456, config);
    
    // 登录
    V2TIMManager::GetInstance()->Login(
        V2TIMString("user123"),
        V2TIMString("userSig"),
        new V2TIMCallback(
            []() { /* 成功 */ },
            [](int code, const V2TIMString& msg) { /* 失败 */ }
        )
    );
    
    // 发送消息
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

### Dart 使用示例

```dart
import 'package:tim2tox_dart/tim2tox_dart.dart';

// 创建服务
final ffiService = FfiChatService(
  preferencesService: prefsAdapter,
  loggerService: loggerAdapter,
  bootstrapService: bootstrapAdapter,
);

// 初始化
await ffiService.init();

// 登录
await ffiService.login(userId: userID, userSig: userSig);

// 发送消息
await ffiService.sendMessage(peerId, "Hello");

// 监听消息
ffiService.messages.listen((message) {
  print('Received: ${message.text}');
});
```

## 相关文档

- [开发指南](DEVELOPMENT_GUIDE.md) - 如何添加新功能和扩展
- [Tim2Tox 架构](ARCHITECTURE.md) - 整体架构设计
- [Tim2Tox FFI 兼容层](FFI_COMPAT_LAYER.md) - Dart* 函数兼容层说明
