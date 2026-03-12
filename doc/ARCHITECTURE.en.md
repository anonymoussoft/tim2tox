# Tim2Tox Architecture
> Language: [Chinese](ARCHITECTURE.md) | [English](ARCHITECTURE.en.md)


This document describes the overall architectural design, module division, data flow diagram and key design decisions of Tim2Tox.

## Contents

- [Overall architecture](#overall-architecture)
- [Module division](#module-division)
- [Data stream](#data-flow)
- [Key Design Decision](#key-design-decisions)
- [Interface Abstraction](#interface-abstraction)
- [Dependency Injection](#dependency-injection)

## Overall architecture

Tim2Tox adopts a layered architecture, which is divided from the bottom to the top:

```
┌─────────────────────────────────────────────────────────┐
│ Flutter/Dart application layer │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐            │
│  │   UIKit  │  │  Client  │  │  Custom  │            │
│  │   SDK    │  │   Code   │  │   Code   │            │
│  └────┬─────┘  └────┬─────┘  └────┬─────┘            │
└───────┼──────────────┼─────────────┼────────────────────┘
        │              │             │
        └──────────────┴─────────────┘
                       │
        ┌──────────────▼──────────────┐
        │ Dart SDK Platform layer │
        │  ┌────────┐  ┌──────────┐  │
        │  │Tim2Tox │  │  Service │  │
        │  │Platform│  │  Layer   │  │
        │  └───┬────┘  └────┬─────┘  │
        │      │             │        │
        │  ┌───▼─────────────▼─────┐ │
        │  │    FFI Bindings        │ │
        │  └───────────┬────────────┘ │
        └──────────────┼──────────────┘
                       │
        ┌──────────────▼──────────────┐
        │ C FFI interface layer │
        │  ┌────────┐  ┌──────────┐  │
        │  │tim2tox │  │   Dart*  │  │
        │  │  FFI   │  │ Compat   │  │
        │  └───┬────┘  └────┬─────┘  │
        └──────┼─────────────┼────────┘
               │             │
        ┌──────▼─────────────▼──────┐
        │ V2TIM API implementation layer │
        │  ┌────────┐  ┌──────────┐ │
        │  │Manager│  │  Manager │ │
        │  │ Impl  │  │   Impl   │ │
        │  └───┬───┘  └────┬─────┘ │
        └──────┼───────────┼────────┘
               │           │
        ┌──────▼───────────▼──────┐
        │ Tox core layer │
        │  ┌────────┐  ┌────────┐ │
        │  │ Tox    │  │ ToxAV  │ │
        │  │Manager │  │Manager │ │
        │  └───┬────┘  └────┬───┘ │
        └──────┼────────────┼──────┘
               │            │
        ┌──────▼────────────▼──────┐
        │      c-toxcore            │
        │ (P2P communication library) │
        └───────────────────────────┘
```

## Module division

### 1. V2TIM API layer (`include/`, `source/`)

**Responsibilities**: Implement V2TIM API compatible with Tencent Cloud IM SDK

**Main modules**:
- `V2TIMManager`: core manager, providing SDK initialization, login, and obtaining other managers
- `V2TIMMessageManager`: Message management, create, send and receive messages
- `V2TIMFriendshipManager`: Friend management, add, delete and query friends
- `V2TIMGroupManager`: Group management, create, join and manage groups
- `V2TIMConversationManager`: Session management, query and delete sessions
- `V2TIMSignalingManager`: signaling management, audio and video call invitation
- `V2TIMCommunityManager`: Community management, topic and permission group management

**Design Principles**:
- Fully compatible with V2TIM API of Tencent Cloud IM SDK
- Use callback mechanism to handle asynchronous operations
- All operations are thread-safe

### 2. Tox core layer (`source/ToxManager.*`)

**Responsibilities**: Manage Tox instances and lifecycle, handle underlying P2P communication

**Main components**:
- `ToxManager`: Manage Tox instances and handle core functions such as friends, messages, and groups
- `ToxAVManager`: Manage audio and video calls (optional)
- `IrcClientManager`: IRC channel bridge management (optional)

**Design Principles**:
- Single Tox instance management
- Event-driven architecture
- Automatic reconnection and error recovery

#### Group chat implementation

Tim2Tox supports two Tox group chat APIs at the same time:

**Group API (New API)**:
- Created using `tox_group_new`
- Support `chat_id` persistence (32 bytes)
- Can rejoin via `chat_id`
- Complete functions, supporting group announcements, member management, etc.
- **Recommended to use**

**Conference API (old API)**:
- Created using `tox_conference_new`
- Does not support `chat_id`, relies on savedata recovery
- Can only be joined via friend invitation
- Function is relatively simple
- **For compatibility only**

**Mapping relationship management**:
- Bidirectional mapping between `group_id` ↔ `group_number` ↔ `chat_id`
- Stored in memory and needs to be restored after reboot
- The mapping relationship can be reconstructed through `chat_id`

**Key code location**:
- `tim2tox/source/V2TIMGroupManagerImpl.cpp` - Group management implementation
- `tim2tox/source/V2TIMManagerImpl.cpp` - Group recovery mechanism
- `tim2tox/source/ToxManager.cpp` - Tox group operation package

### 3. FFI interface layer (`ffi/`)

**Responsibilities**: Provide C interface for Dart FFI to call

**Main components**:
- `tim2tox_ffi.h/cpp`: Main FFI interface, providing high-level API
- `dart_compat_layer.h/cpp`: Dart* function compatibility layer main entrance (modularized)
- `dart_compat_internal.h`: Shared declarations and forward declarations
- **Modular implementation** (13 module files):
  - `dart_compat_utils.cpp`: Utility functions and global variables
  - `dart_compat_listeners.cpp`: Listener implementation and callback registration
  - `dart_compat_callbacks.cpp`: Callback class implementation
  - `dart_compat_sdk.cpp`: SDK initialization and authentication
  - `dart_compat_message.cpp`: Message related functions
  - `dart_compat_friendship.cpp`: Friend related functions
  - `dart_compat_conversation.cpp`: Conversation related functions
  - `dart_compat_group.cpp`: Group related functions
  - `dart_compat_user.cpp`: User related functions
  - `dart_compat_signaling.cpp`: Signaling related functions
  - `dart_compat_community.cpp`: Community related functions
  - `dart_compat_other.cpp`: Other miscellaneous functions
- `callback_bridge.h/cpp`: Callback bridging mechanism to send C++ events to Dart
- `json_parser.h/cpp`: JSON message construction and parsing tool

**Design Principles**:
- C interface, no C++ dependencies
- Thread-safe callback mechanism
- Messaging in JSON format
- **Modular design**: The code is split into independent modules according to functions to improve maintainability

### 4. Dart binding layer (`dart/lib/`)

**Responsibilities**: Provide Flutter/Dart bindings and high-level APIs

**Main components**:
- `ffi/tim2tox_ffi.dart`: Low-level FFI binding, directly mapping C functions
- `service/ffi_chat_service.dart`: Advanced service layer, managing message history, polling, status
- `sdk/tim2tox_sdk_platform.dart`: SDK Platform implementation, routing UIKit SDK calls
- `service/toxav_service.dart`: ToxAV management and instance routing
- `service/call_bridge_service.dart`: signaling and ToxAV bridging
- `interfaces/`: Abstract interface definition, support dependency injection

**Design Principles**:
- Clear layer separation
- Use Future/Stream for asynchronous operations
- Interface abstraction, support dependency injection

## Integration solution

Tim2Tox supports three access forms:

- **Pure binary replacement**: only replace the dynamic library, do not register the Platform
- **Platform interface scheme**: only routed through `Tim2ToxSdkPlatform`
- **Hybrid Architecture**: Preserves both binary replacement and Platform paths

Among them, **Flutter Echo Client currently uses a hybrid architecture**.

### Solution 1: Pure binary replacement solution

**Features**:
- ✅ **Zero Dart Code Modification**: No setup required `TencentCloudChatSdkPlatform.instance`
- ✅ **Fully compatible with native SDK**: use `TIMManager.instance` → `NativeLibraryManager` → Dart* function
- ✅ **Just replace the dynamic library**: Replace `dart_native_imsdk` with `libtim2tox_ffi`

**Call path**:
```
UIKit SDK
  ↓
TIMManager.instance
  ↓
NativeLibraryManager (native SDK call path)
  ↓
bindings.DartXXX(...) (FFI dynamic symbol lookup)
  ↓
libtim2tox_ffi.dylib (replaced dynamic library)
  ↓
dart_compat_layer.cpp (Dart* function implementation)
  ↓
V2TIM*Manager (C++ API implementation)
  ↓
ToxManager (Tox core)
  ↓
c-toxcore (P2P communication)
```

**Key components**:
- `tim2tox/ffi/dart_compat_layer.cpp` - implements all Dart* functions
- `tim2tox/ffi/callback_bridge.cpp` - callback bridging mechanism
- `tencent_cloud_chat_sdk-8.7.7201/lib/native_im/bindings/native_library_manager.dart` - Dynamic library loading

**Detailed documentation**: See [Tim2Tox binary replacement](BINARY_REPLACEMENT.en.md)

### Solution 2: Platform interface solution (alternative)

**Features**:
- Requires setting `TencentCloudChatSdkPlatform.instance = Tim2ToxSdkPlatform(...)`
- Use the advanced service layer (FfiChatService) to provide richer functions
- Need to modify Dart layer code

**Call path**:
```
UIKit SDK
  ↓
TencentCloudChatSdkPlatform.instance (Tim2ToxSdkPlatform)
  ↓
FfiChatService (advanced service layer)
  ↓
Tim2ToxFfi (FFI binding)
  ↓
tim2tox_ffi_* (C FFI interface)
  ↓
V2TIM*Manager (C++ API implementation)
  ↓
ToxManager (Tox core)
  ↓
c-toxcore (P2P communication)
```

**Key components**:
- `tim2tox/dart/lib/sdk/tim2tox_sdk_platform.dart` - Platform interface implementation
- `tim2tox/dart/lib/service/ffi_chat_service.dart` - Advanced service layer
- `tim2tox/dart/lib/ffi/tim2tox_ffi.dart` - FFI binding

### Solution 3: Hybrid architecture (currently used by Flutter Echo Client)**Features**:
- `main()` executes `setNativeLibraryName('tim2tox_ffi')` first
- The client registers `Tim2ToxSdkPlatform` in HomePage
- NativeLibraryManager continues to be responsible for most binary replacement calls
- Platform path is responsible for historical messages, custom callbacks, calls and some advanced capabilities

**Extra components**:
- `customCallbackHandler` in `sdk/tim2tox_sdk_platform.dart`
- `service/toxav_service.dart`, `service/call_bridge_service.dart`
- `service/tuicallkit_adapter.dart`, `service/tuicallkit_tuicore_integration.dart`

### Plan comparison

| Features | Pure binary replacement | Platform interface scheme | Hybrid architecture |
|------|----------------|-------------------|----------|
| Dart code modification | ❌ Not required | ✅ Required | ✅ Required |
| Deployment Complexity | ✅ Simple | ⚠️ Medium | ⚠️ Medium |
| Function richness | ⚠️ Basic functions | ✅ Advanced functions | ✅ Complete |
| Maintenance Cost | ✅ Low | ⚠️ Medium | ⚠️ Medium High |
| Compatibility | ✅ Fully compatible with native SDK | ⚠️ Need to adapt to UIKit SDK | ✅ The current client has been verified |
| Applicable scenarios | Quick verification, minimal access | Independent SDK packaging | Client production access |

## Data flow

### SDK calling path (binary replacement solution)

```
UIKit SDK Calls
  ↓
TIMManager.instance
  ↓
NativeLibraryManager.bindings.DartXXX(...)
  ↓
FFI dynamically finds symbols (in libtim2tox_ffi)
  ↓
dart_compat_layer.cpp (Dart* function implementation)
  ↓
V2TIM*Manager (C++ API implementation)
  ↓
ToxManager (Tox core)
  ↓
c-toxcore (P2P communication)
```

### SDK calling path (Platform interface solution)

```
UIKit SDK Calls
  ↓
TencentCloudChatSdkPlatform.instance (Tim2ToxSdkPlatform)
  ↓
FfiChatService (advanced service layer)
  ↓
Tim2ToxFfi (FFI binding)
  ↓
tim2tox_ffi_* (C FFI interface)
  ↓
V2TIM*Manager (C++ API implementation)
  ↓
ToxManager (Tox core)
  ↓
c-toxcore (P2P communication)
```

### Message sending path

```
User input message
  ↓
UIKit Message Input
  ↓
Tim2ToxSdkPlatform.sendMessage()
  ↓
ChatMessageProvider.sendText() / sendImage() / sendFile()
  ↓
FfiChatService.sendText() / sendGroupText() / sendFile() / sendGroupFile()
  ↓
tim2tox_ffi_send_c2c_text() / tim2tox_ffi_send_group_text()
  ↓
V2TIMMessageManagerImpl::SendMessage()
  ↓
ToxManager::SendMessage()
  ↓
tox_friend_send_message() (c-toxcore)
  ↓
P2P network transmission
```

### Message receiving path

```
P2P network reception
  ↓
tox_friend_message() (c-toxcore callback)
  ↓
ToxManager::OnFriendMessage()
  ↓
V2TIMMessageManagerImpl::OnRecvNewMessage()
  ↓
Listener callback (DartAdvancedMsgListenerImpl)
  ↓
SendCallbackToDart() (JSON message)
  ↓
Dart ReceivePort
  ↓
NativeLibraryManager._handleNativeMessage()
  ↓
Tim2ToxSdkPlatform event distribution
  ↓
UIKit Listeners
  ↓
UI updates
```

### Callback mechanism

Tim2Tox implements the notification mechanism from C++ to the Dart layer through the `SendCallbackToDart` function.

#### Initialization process

1. **Dart layer registration SendPort**:
   ```dart
   NativeLibraryManager.registerPort();
   // Internally calls: bindings.DartRegisterSendPort(_receivePort.sendPort.nativePort);
   ```

2. **C++ layer storage SendPort**:
   ```cpp
   void DartRegisterSendPort(int64_t send_port) {
       g_dart_port = static_cast<Dart_Port>(send_port);
   }
   ```

3. **C++ layer sends message**:
   ```cpp
   SendCallbackToDart("callback_type", json_data, user_data);
   ```

4. **Dart layer receives message**:
   ```dart
   _receivePort.listen((dynamic message) {
       _handleNativeMessage(message);
   });
   ```

#### Callback type

**1. `apiCallback` - API call result callback**

Used to return the results of an asynchronous API call:

```json
{
  "callback": "apiCallback",
  "user_data": "unique_id",
  "code": 0,
  "desc": "success",
  "json_param": "{...}"  // Optional
}
```

**2. `globalCallback` - Global event callback**

Used to notify various global events (network status, message reception, friend changes, etc.):

```json
{
  "callback": "globalCallback",
  "callbackType": 7,  // GlobalCallbackType enumeration value
  "json_message": "{...}",  // or other fields
  "user_data": "..."
}
```

**3. `groupQuitNotification` - Group exit notification**

Used to notify the Dart layer to clean up the group status:

```json
{
  "callback": "groupQuitNotification",
  "group_id": "tox_1",
  "user_data": "..."
}
```

#### Complete callback process

```
C++ layer events
  ↓
Listener implementation (Dart*ListenerImpl)
  ↓
BuildGlobalCallbackJson() / BuildApiCallbackJson()
  ↓
SendCallbackToDart()
  ↓
Dart_PostCObject_DL()
  ↓
Dart ReceivePort
  ↓
NativeLibraryManager._handleNativeMessage()
  ↓
Dart layer callback processing
```

**Key code location**:
- `tim2tox/ffi/callback_bridge.cpp:SendCallbackToDart()`
- `tim2tox/ffi/json_parser.cpp:BuildGlobalCallbackJson()`
- `tim2tox/ffi/json_parser.cpp:BuildApiCallbackJson()`

## Key Design Decisions

### 1. Interface abstraction and dependency injection

**Decision**: Tim2Tox injects client dependencies through interfaces instead of directly relying on client code

**Reason**:
- Tim2Tox is completely independent and can be reused by any Flutter client
- The client can freely choose the implementation method (SharedPreferences, custom storage, etc.)
- Easy to test and simulate

**Implementation**:
- `ExtendedPreferencesService`: Preference interface
- `LoggerService`: Log interface
- `BootstrapService`: Bootstrap node interface
- `EventBusProvider`: event bus interface
- `ConversationManagerProvider`: Session manager interface

### 2. Dual data path

**Decision**: Support both SDK Events and Data Streams data paths

**Reason**:
- SDK Events: Listener mechanism compatible with UIKit SDK
- Data Streams: Provides more flexible data stream processing

**Implementation**:
- SDK Events: routed to UIKit SDK Listeners via `Tim2ToxSdkPlatform`
- Data Streams: Provides message streaming via `FfiChatService.messages` Stream

### 3. Message ID management

**Decision**: Use `timestamp_userID` format as message ID

**Reason**:
- Ensure message ID is unique
- Easy to sort and query
- Compatible with UIKit SDK expectations

**Implementation**:
- Generated when sending message: `"${timestamp}_${userID}"`
- Parse timestamp and user ID when message is received

### 4. Failure message processing

**Decision**: Implement a complete failure message detection and persistence mechanism

**Reason**:
- P2P networks may be unstable
- Need to detect offline status and timeout
- Need to persist failure messages for recovery

**Implementation**:
- Offline detection: Immediately detect whether the contact is online
- Timeout detection: 5 seconds for text messages, dynamically calculated based on size for file messages
- Persistence: Use SharedPreferences to store failure messages
- Recovery: Automatically restore the failed message status after the client restarts

### 5. Binary replacement solution

**Decision**: Implement Dart* function compatibility layer to support binary replacement

**Reason**:
- Minimize Dart layer code modifications
- Completely retain the calling method of NativeLibraryManager
- Supports gradual migration

**Implementation**:
- `dart_compat_layer.cpp`: Export all Dart* functions
- `callback_bridge.cpp`: Implement callback bridging mechanism
- `json_parser.cpp`: Implement JSON message construction and parsing

### 6. Asynchronous operation processing

**Decision**: Use asynchronous callbacks for all time-consuming operations

**Reason**:
- Avoid blocking the main thread
- Provide better user experience
- Asynchronous model compatible with UIKit SDK

**Implementation**:
- C++ layer: use `V2TIMCallback` and `V2TIMValueCallback`
- Dart layer: use `Future` and `Stream`
- FFI layer: Deliver results via JSON message

## Interface abstraction

### Required interface

The client must implement the following interface:

#### ExtendedPreferencesService

Preferences service for persisting data.

```dart
abstract class ExtendedPreferencesService {
  Future<String?> getString(String key);
  Future<bool> setString(String key, String value);
  // ... other methods
}
```

#### LoggerService

Log service, used to output logs.

```dart
abstract class LoggerService {
  void log(String message);
  void logError(String message, Object error, StackTrace stack);
  void logWarning(String message);
  void logDebug(String message);
}
```

#### BootstrapService

Bootstrap node service for Tox network connection.

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

### Optional interface

Clients can optionally implement the following interfaces to enable advanced functionality:

#### EventBusProvider

Event bus provider for inter-component communication.

```dart
abstract class EventBusProvider {
  EventBus get eventBus;
}
```

#### ConversationManagerProvider

Session manager provider for session management.

```dart
abstract class ConversationManagerProvider {
  ConversationManager get conversationManager;
}
```

## Dependency injection

### Initialization process

```dart
// 1. Create an interface adapter
final prefsAdapter = SharedPreferencesAdapter(await SharedPreferences.getInstance());
final loggerAdapter = AppLoggerAdapter();
final bootstrapAdapter = BootstrapNodesAdapter(await SharedPreferences.getInstance());

// 2. Create a service
final ffiService = FfiChatService(
  preferencesService: prefsAdapter,
  loggerService: loggerAdapter,
  bootstrapService: bootstrapAdapter,
);

// 3. Initialize service
await ffiService.init();

// 4. Set up SDK Platform
TencentCloudChatSdkPlatform.instance = Tim2ToxSdkPlatform(
  ffiService: ffiService,
  eventBusProvider: eventBusAdapter,        // Optional
  conversationManagerProvider: convAdapter,  // Optional
);
```

### Adapter mode

The client maps the abstract interface of Tim2Tox to the concrete implementation through the adapter:

```dart
class SharedPreferencesAdapter implements ExtendedPreferencesService {
  final SharedPreferences _prefs;
  SharedPreferencesAdapter(this._prefs);
  
  @override
  Future<String?> getString(String key) async => _prefs.getString(key);
  
  // ...implement other methods
}
```

## Threading model

### C++ layer- **Main Thread**: All V2TIM API calls should be made on the main thread
- **Tox thread**: Tox event callback is executed in the Tox thread
- **Callback Thread**: Sent to Dart thread via `Dart_PostCObject_DL`

### Dart layer

- **UI Thread**: All UI operations and SDK calls are in the UI thread
- **Isolate**: optional, used for time-consuming operations

## Memory management

### C++ layer

- Use smart pointers (`std::shared_ptr`, `std::unique_ptr`)
- String uses `V2TIMString` to manage lifecycle
- The callback object's lifecycle is managed by the caller

### Dart layer

- Use Dart’s garbage collection mechanism
- FFI strings use `toNativeUtf8()` and `malloc.free()`
- Stream is managed using `StreamController`

## Error handling

### C++ layer

- Use error codes and error messages
- Return error information through callback
- Keep detailed logs

### Dart layer

- Exception handling using `Future`
- Return error code and description via `V2TimCallback`
- Provide user-friendly error prompts

## Performance optimization

### Message processing

- Process messages in batches
- Use message queue to avoid blocking
- Asynchronous processing of time-consuming operations

### Network optimization

- Connection pool management
- Automatic reconnection mechanism
- Network status monitoring

### Memory optimization

- Object pool reuse
- Release unused resources promptly
- Avoid memory leaks

## Related documents

- [API Reference](API_REFERENCE.en.md) - Complete API documentation
- [Development Guide](DEVELOPMENT_GUIDE.en.md) - Development Guide
- [Tim2Tox FFI compatibility layer](FFI_COMPAT_LAYER.en.md) - Dart* function compatibility layer description
- [Tim2Tox Bootstrap and Polling](BOOTSTRAP_AND_POLLING.en.md) - Bootstrap nodes, network establishment and poll loop
- [Tim2Tox ToxAV and Signaling](TOXAV_AND_SIGNALING.en.md) - Calls and instance routing
