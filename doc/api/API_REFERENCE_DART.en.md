# Tim2Tox API Reference — Dart Package
> Language: [Chinese](API_REFERENCE_DART.md) | [English](API_REFERENCE_DART.en.md)

This document is the Dart package section of [API_REFERENCE.en.md](API_REFERENCE.en.md).

## Dart Package API

The Dart package provides a high-level API, encapsulates FFI calls, and provides an easier-to-use interface.

### Tim2ToxFfi

The underlying FFI binding class maps directly to C FFI functions.

**File**: `dart/lib/ffi/tim2tox_ffi.dart`

```dart
// Open FFI library
static Tim2ToxFfi open()

// Set unified log file
void setLogFile(String path)

// Get the current instance ID
int getCurrentInstanceId()

// initialization
int init()

// Login
int login(String userID, String userSig)

// Poll native events
int pollText(int instanceId, List<int> buffer)

// Get logged in user
int getLoginUser(List<int> buffer)

// ToxAV
int avInitialize(int instanceId)
void avShutdown(int instanceId)
void avIterate(int instanceId)
```

### FfiChatService

Advanced service layer manages message history, polling, status, etc.

**File**: `dart/lib/service/ffi_chat_service.dart`

```dart
class FfiChatService {
  // Constructor
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

  // initialization
  Future<void> init({String? profileDirectory})

  // Login
  Future<void> login({required String userId, required String userSig})

  // Start polling
  Future<void> startPolling()

  // Send text message
  Future<String?> sendMessage(String peerId, String text)

  // message flow
  Stream<ChatMessage> get messages

  // Connection status stream
  Stream<bool> get connectionStatusStream

  // Get message history
  List<ChatMessage> getHistory(String id)

  // Set input status
  void setTyping(String userID, bool typing)
}
```

### Tim2ToxSdkPlatform

Implement the `TencentCloudChatSdkPlatform` interface to route UIKit SDK calls to tim2tox.

**File**: `dart/lib/sdk/tim2tox_sdk_platform.dart`

```dart
class Tim2ToxSdkPlatform extends TencentCloudChatSdkPlatform {
  // Constructor
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
  Future<V2TimValueCallback<V2TimMessage>> sendMessage({...})

  // Get the friends list
  @override
  Future<V2TimValueCallback<List<V2TimFriendInfo>>> getFriendList()

  // Add friend
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
