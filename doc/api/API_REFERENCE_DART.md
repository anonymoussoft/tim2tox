# Tim2Tox API 参考 — Dart 包
> 语言 / Language: [中文](API_REFERENCE_DART.md) | [English](API_REFERENCE_DART.en.md)

本文档为 [API_REFERENCE.md](API_REFERENCE.md) 的 Dart 包部分。

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
