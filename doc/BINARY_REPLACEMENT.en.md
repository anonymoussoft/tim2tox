# Tim2Tox Binary Replacement
> Language: [Chinese](BINARY_REPLACEMENT.md) | [English](BINARY_REPLACEMENT.en.md)


This document details how to use the underlying Tox binary implementation to replace the TIMSDK binary to achieve an integration solution with zero Dart code modification.

## Overview

The binary replacement scheme (BINARY REPLACEMENT MODE) is a scheme that realizes backend switching by replacing dynamic library files, so that the Dart layer code can be switched from Tencent Cloud IM SDK to the Tox P2P protocol without any modification at all.

**Current usage status**: ✅ **toxee uses hybrid mode (Binary Replacement + Platform interface)**

**Configuration method**:
- Call the `setNativeLibraryName('tim2tox_ffi')` configuration library name at the earliest stage of `main()`
- Optional: Set `TencentCloudChatSdkPlatform.instance = Tim2ToxSdkPlatform(...)` to enable advanced features

## Core Principles

### 1. Dynamic library replacement

**Key File**: `tencent_cloud_chat_sdk-8.7.7201/lib/native_im/bindings/native_library_manager.dart`

In `NativeLibraryManager`, configure the dynamic library name through `setNativeLibraryName()` (the default is `dart_native_imsdk` of the original SDK):

```dart
// The default value is the original SDK, which the caller overrides at the earliest stage of main()
String _nativeLibName = 'dart_native_imsdk';
void setNativeLibraryName(String name) {
  _nativeLibName = name;
}

final DynamicLibrary _dylib = () {
  final libName = _nativeLibName;
  if (Platform.isMacOS) {
    try {
      return DynamicLibrary.open('$libName.framework/Versions/A/$libName');
    } catch (e) {
      return DynamicLibrary.open('lib$libName.dylib');
    }
  }
  // ...processing for other platforms
}();
```

**How to use** (in the earliest days of `main()`, before any SDK calls):
```dart
import 'package:tencent_cloud_chat_sdk/native_im/bindings/native_library_manager.dart';
setNativeLibraryName('tim2tox_ffi');
```

**Important**: `_dylib` is a top-level `final` variable that is lazily initialized on first access. `setNativeLibraryName()` must be called before any `NativeLibraryManager.bindings` is used, otherwise the default `dart_native_imsdk` will be loaded.

**Compile product**:
- macOS: `libtim2tox_ffi.dylib`
- Linux: `libtim2tox_ffi.so`
- iOS: `tim2tox_ffi.framework` or `libtim2tox_ffi.dylib`
- Android: `libtim2tox_ffi.so`
- Windows: `tim2tox_ffi.dll`

### 2. Function signature exactly matches

**Key File**: `tim2tox/ffi/dart_compat_layer.cpp`

The signature of all Dart* functions must be exactly as defined in `native_imsdk_bindings_generated.dart`:

```cpp
// Example: DartInitSDK function
extern "C" {
    int DartInitSDK(const char* json_init_param, void* user_data) {
        // 1. Parse JSON parameters
        std::string json_str(json_init_param);
        std::string sdk_app_id_str = ExtractJsonValue(json_str, "sdk_config_sdk_app_id");
        // ...
        
        // 2. Call V2TIM SDK API
        V2TIMManager::GetInstance()->InitSDK(sdkAppID, config);
        
        // 3. Return the result through callback
        SendApiCallbackResult(user_data, 0, "OK");
        return 0;
    }
}
```

**Function signature source**: `native_imsdk_bindings_generated.dart` is automatically generated from the header file of the native SDK through `ffigen` to ensure that the signature matches exactly.

### 3. FFI dynamic symbol lookup

**Key File**: `tencent_cloud_chat_sdk-8.7.7201/lib/native_im/bindings/native_imsdk_bindings_generated.dart`

The Dart layer dynamically looks up symbols via FFI:

```dart
class NativeImsdkBindings {
  final ffi.Pointer<T> Function<T extends ffi.NativeType>(String symbolName) _lookup;
  
  NativeImsdkBindings(ffi.DynamicLibrary dynamicLibrary)
      : _lookup = dynamicLibrary.lookup;
  
  int DartInitSDK(ffi.Pointer<ffi.Char> json_init_param, ffi.Pointer<ffi.Void> user_data) {
    return _DartInitSDK(json_init_param, user_data);
  }
  
  late final _DartInitSDKPtr = _lookup<ffi.NativeFunction<...>>('DartInitSDK');
  late final _DartInitSDK = _DartInitSDKPtr.asFunction<int Function(...)>();
}
```

When `_dylib` points to `libtim2tox_ffi.dylib`, `_lookup('DartInitSDK')` looks for the symbol in `libtim2tox_ffi.dylib` instead of the native `dart_native_imsdk`.

### 4. Callback mechanism

**Key File**: `tim2tox/ffi/callback_bridge.cpp`

The C++ layer sends the event back to the Dart layer via `Dart_PostCObject_DL`:

```cpp
void SendCallbackToDart(const char* callback_type, const std::string& json_data, void* user_data) {
    // 1. Build a JSON message
    std::string message = json_data; // Already contains the "callback" field
    
    // 2. Create Dart_CObject
    Dart_CObject cobj;
    cobj.type = Dart_CObject_kString;
    char* message_cstr = static_cast<char*>(malloc(message.length() + 1));
    std::memcpy(message_cstr, message.c_str(), message.length());
    cobj.value.as_string = message_cstr;
    
    // 3. Send to Dart layer
    Dart_PostCObject_DL(g_dart_port, &cobj);
}
```

**Initialization process**:
1. The Dart layer calls `DartInitDartApiDL(data)` to initialize the Dart API
2. Dart layer calls `DartRegisterSendPort(port)` to register SendPort
3. The C++ layer stores the port number for subsequent callback sending

## Complete calling process

### SDK initialization process

```
Dart layer (TIMManager.initSDK)
  ↓
NativeLibraryManager.bindings.DartInitSDK(...)
  ↓
FFI dynamically looks for symbols 'DartInitSDK' (in libtim2tox_ffi.dylib)
  ↓
C++ layer (dart_compat_layer.cpp::DartInitSDK)
  ↓
Parse JSON parameters (json_parser.cpp)
  ↓
V2TIMManager::GetInstance()->InitSDK(...)
  ↓
ToxManager::getInstance().init(...)
  ↓
tox_new() (c-toxcore)
  ↓
Callback: SendApiCallbackResult(user_data, 0, "OK")
  ↓
Dart layer (ReceivePort receiving callback)
  ↓
NativeLibraryManager._handleNativeMessage()
  ↓
Business code processing results
```

### Message sending process

```
Dart layer (TIMMessageManager.sendMessage)
  ↓
NativeLibraryManager.bindings.DartSendMessage(...)
  ↓
C++ layer (dart_compat_layer.cpp::DartSendMessage)
  ↓
V2TIMMessageManagerImpl::SendMessage(...)
  ↓
ToxManager::SendMessage(...)
  ↓
tox_friend_send_message() (c-toxcore)
  ↓
P2P network transmission
```

### Message receiving process

```
P2P network reception
  ↓
tox_friend_message() callback (c-toxcore)
  ↓
ToxManager::OnFriendMessage(...)
  ↓
V2TIMMessageManagerImpl::OnRecvNewMessage(...)
  ↓
DartAdvancedMsgListenerImpl::OnRecvNewMessage(...)
  ↓
BuildGlobalCallbackJson() (json_parser.cpp)
  ↓
SendCallbackToDart("globalCallback", json_data, nullptr)
  ↓
Dart_PostCObject_DL(g_dart_port, &cobj)
  ↓
Dart layer (ReceivePort receiving callback)
  ↓
NativeLibraryManager._handleNativeMessage()
  ↓
NativeLibraryManager._handleGlobalCallback()
  ↓
Business code processes messages
```

## Key components

### 1. dart_compat_layer.cpp/h

**Location**: `tim2tox/ffi/dart_compat_layer.cpp`

**Responsibilities**: Implement all Dart* functions and provide an interface that is fully compatible with the native SDK.

**Main functions**:
- `DartInitSDK`: SDK initialization
- `DartLogin`: User login
- `DartSendMessage`: Send message
- `DartGetConversationList`: Get conversation list
- `DartGetFriendList`: Get friends list
- Wait...

**Implementation Mode**:
```cpp
int DartXXX(const char* json_param, void* user_data) {
    // 1. Parse JSON parameters
    std::string json_str(json_param);
    std::string field1 = ExtractJsonValue(json_str, "field1");
    
    // 2. Call V2TIM SDK API
    V2TIMManager::GetInstance()->GetXXXManager()->XXX(...);
    
    // 3. Return results through callback (asynchronous)
    SendApiCallbackResult(user_data, code, desc);
    return 0;
}
```

### 2. callback_bridge.cpp/h

**Location**: `tim2tox/ffi/callback_bridge.cpp`

**Responsibilities**: Implement the callback bridging mechanism to send C++ events to the Dart layer.

**Main functions**:
- `DartInitDartApiDL`: Initialize Dart API
- `DartRegisterSendPort`: Register Dart SendPort
- `SendCallbackToDart`: Send callback message to Dart layer

### 3. json_parser.cpp/h

**Location**: `tim2tox/ffi/json_parser.cpp`

**Responsibilities**: Implement JSON message construction and parsing tools.

**Main functions**:
- `BuildGlobalCallbackJson`: Build globalCallback JSON message
- `BuildApiCallbackJson`: Build apiCallback JSON message
- `ExtractJsonValue`: Extract JSON values
- `ParseJsonString`: Parse JSON string

### 4. Listener implementation

**Location**: `tim2tox/ffi/dart_compat_layer.cpp`

**Responsibilities**: Implement the V2TIM Listener interface and convert Tox events into JSON messages.

**Main Listener**:
- `DartSDKListenerImpl`: SDK event monitoring (connection status, user status, etc.)
- `DartAdvancedMsgListenerImpl`: Message event monitoring (new messages, message modifications, etc.)
- `DartFriendshipListenerImpl`: Friend event monitoring (friend addition, deletion, etc.)
- `DartConversationListenerImpl`: Session event monitoring (session update, etc.)
- `DartGroupListenerImpl`: Group event monitoring (group prompts, etc.)
- `DartSignalingListenerImpl`: Signaling event monitoring (audio and video invitations, etc.)
- `DartCommunityListenerImpl`: Community event monitoring (topic creation, etc.)

## Differences from Platform interface scheme

### Plan comparison

| Features | Binary replacement scheme | Platform interface scheme | Mixed mode (currently used) |
|------|-------------|------------------|-------------------|
| **Dart code modification** | `setNativeLibraryName` only | Requires Platform setup | Both required |
| **Function richness** | Basic functions | Advanced functions | Complete functions |
| **Custom callback** | By `customCallbackHandler` | Implemented directly on Platform | `customCallbackHandler` registered to Platform |
| **Applicable scenarios** | Rapid integration | Advanced features required | Production environment |

### Mixed mode (currently used by toxee)

toxee uses both binary replacement and the Platform interface:

1. `setNativeLibraryName('tim2tox_ffi')` — Load `libtim2tox_ffi.dylib`
2. `TencentCloudChatSdkPlatform.instance = Tim2ToxSdkPlatform(...)` — Enable advanced features
3. `Tim2ToxSdkPlatform` constructor registration `NativeLibraryManager.customCallbackHandler` — takes over custom callback

### customCallbackHandler mechanism

The default switch branch of `NativeLibraryManager._handleNativeCallback` in the SDK is delegated to `customCallbackHandler`:

```dart
// Within the NativeLibraryManager class
static Future<void> Function(
  String callbackName,
  Map<String, dynamic> data,
  Map<String, ApiCallback> apiCallbackMap,
)? customCallbackHandler;

// _handleNativeCallback switch:
default:
  if (customCallbackHandler != null) {
    await customCallbackHandler!(callbackName, dataFromNativeMap, _apiCallbackMap);
  }
  break;
```Tim2ToxSdkPlatform registers a handler during construction to handle 3 custom callbacks:
- `clearHistoryMessage` — Clear C2C/Group Chat History
- `groupQuitNotification` — Clean up the status after the group exits
- `groupChatIdStored` — storage group chat_id mapping

### Platform interface solution

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

**Features**:
- Requires setting `TencentCloudChatSdkPlatform.instance = Tim2ToxSdkPlatform(...)`
- Need to modify Dart layer code
- Use the advanced service layer (FfiChatService) to provide richer functions
- Supports advanced functions such as message history management, event polling, status management, etc.

**Key Documents**:
- `tim2tox/dart/lib/sdk/tim2tox_sdk_platform.dart` - Platform interface implementation
- `tim2tox/dart/lib/service/ffi_chat_service.dart` - Advanced Service Tier
- `tim2tox/dart/lib/ffi/tim2tox_ffi.dart` - FFI binding

### Pure binary replacement solution

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

**Features**:
- **Zero Dart code changes**: No need to set `TencentCloudChatSdkPlatform.instance`
- **Fully Compatible**: Function signature and callback format exactly match the native SDK
- **Just replace the dynamic library**: Replace `dart_native_imsdk` with `libtim2tox_ffi`
- **Applicable Scenarios**: Quick verification, minimized access, isolated debugging

**Key Documents**:
- `tencent_cloud_chat_sdk-8.7.7201/lib/native_im/bindings/native_library_manager.dart` - Dynamic library loading
- `tim2tox/ffi/dart_compat_layer.cpp` - Dart* function implementation
- `tim2tox/ffi/callback_bridge.cpp` - Callback bridging mechanism

## Deployment method

### 1. Compile dynamic library

**Location**: `tim2tox/ffi/CMakeLists.txt`

```bash
cd tim2tox
mkdir -p build && cd build
cmake ..
make tim2tox_ffi
```

**Output**:
- macOS: `build/ffi/libtim2tox_ffi.dylib`
- Linux: `build/ffi/libtim2tox_ffi.so`

### 2. Package into application

**macOS**:
```bash
cp tim2tox/build/ffi/libtim2tox_ffi.dylib \
   toxee/build/macos/Build/Products/Debug/toxee.app/Contents/MacOS/
```

**Linux**:
```bash
cp tim2tox/build/ffi/libtim2tox_ffi.so \
   toxee/build/linux/x64/debug/bundle/lib/
```

### 3. Dependency library processing

**libsodium** (Tox’s encryption library dependency):
- macOS: Use `install_name_tool` to modify the dependency path of dynamic libraries
- Linux: Make sure the system has `libsodium` installed or packaged into the application

## Advantages

1. **Minimal Dart code modification**: Just add one line `setNativeLibraryName('tim2tox_ffi')` to `main()`
2. **SDK has no proprietary code**: There are only common extension points (`setNativeLibraryName`, `customCallbackHandler`, `isCustomPlatform`) in the SDK and no tim2tox proprietary logic.
3. **Easy to SDK upgrade**: When upgrading the SDK, you only need to incorporate common extension points and bug fixes (memory management, null safety), and no tim2tox proprietary code is involved.
4. **Full Compatibility**: Function signature and callback format exactly match the native SDK
5. **Testable**: Callback can be injected through the `tim2tox_ffi_inject_callback` FFI function for unit testing

## Limitations

1. **Function signatures must match exactly**: Any mismatch in signatures will result in a runtime error
2. **JSON format must match**: The JSON format of parameters and callbacks must be completely consistent with the native SDK
3. **Callback mechanism dependency**: Depends on the callback mechanism of Dart API DL and needs to be initialized correctly.

## Issue fixed

This section documents issues that were discovered and fixed during the development and testing of the binary replacement solution.

### 1. Fix the process of session top function

**Problem description**:
- `conv_event` field in `OnConversationChanged` event is not set correctly
- Causing the Dart layer to not correctly recognize session update events

**Fix Solution**:
- Correctly set `conv_event = "2"` (conversationEventUpdate) in `DartConversationListenerImpl::OnConversationChanged`
- Make sure `ConversationVectorToJson` contains the `conv_is_pinned` field
- Verify the complete event flow: C++ layer → globalCallback → Dart layer → UIKit SDK

**Fixed code locations**:
- `tim2tox/ffi/dart_compat_listeners.cpp:DartConversationListenerImpl::OnConversationChanged()`
- `tim2tox/ffi/dart_compat_utils.cpp:ConversationVectorToJson()`

**Verification results**:
- ✅ Conversation pin/unpin operations correctly trigger the `OnConversationChanged` event
- ✅ Dart layer correctly parses `conv_event = "2"` and routes to `onConversationChanged`
- ✅ UI correctly updates the pinned status of the conversation list

### 2. Group ID generation logic repair

**Problem description**:
- Use `conference_number` to directly generate group ID (`tox_%u`), resulting in ID reuse conflict
- When the old group is deleted, the new group may reuse the same ID

**Fix Solution**:
- Generate unique ID (`tox_%llu`) using global counter `next_group_id_counter_`
- Check for conflicts and ensure uniqueness
- Correctly clean up mapping relationships when exiting/deleting a group to prevent ID reuse

**Fixed code locations**:
-`tim2tox/source/V2TIMManagerImpl.cpp:CreateGroup()`
-`tim2tox/source/V2TIMManagerImpl.cpp:QuitGroup()`
- `tim2tox/source/V2TIMManagerImpl.cpp:DismissGroup()`

**Verification results**:
- ✅ Generate a unique group ID to avoid reusing historical IDs
- ✅ Correctly clear mapping relationships when exiting/deleting a group

### 3. GetJoinedGroupList and SearchGroups fixed

**Problem description**:
- `GetJoinedGroupList` and `SearchGroups` generate group IDs directly instead of using the established mapping relationship
- The returned group ID is inconsistent with the one when created

**Fix Solution**:
- Prioritize getting the group ID from the mapping of `V2TIMManagerImpl`
- If it does not exist in the mapping, try to obtain it through other methods.
- Ensure that the returned group ID is consistent with the one created

**Fixed code locations**:
- `tim2tox/source/V2TIMGroupManagerImpl.cpp:GetJoinedGroupList()`
- `tim2tox/source/V2TIMGroupManagerImpl.cpp:SearchGroups()`

**Verification results**:
- ✅ `GetJoinedGroupList` and `SearchGroups` use the mapping relationship correctly
- ✅ The returned group ID is consistent with the one when created

## Debugging

### Check symbol export

**macOS**:
```bash
nm -D libtim2tox_ffi.dylib | grep Dart
```

**Linux**:
```bash
nm -D libtim2tox_ffi.so | grep Dart
```

### Check dynamic library dependencies

**macOS**:
```bash
otool -L libtim2tox_ffi.dylib
```

**Linux**:
```bash
ldd libtim2tox_ffi.so
```

### Log output

The C++ layer uses `fprintf(stdout, ...)` and `fprintf(stderr, ...)` to output logs, which can be seen in the Flutter console.

## Related documents

- [Tim2Tox FFI Compatibility Layer](FFI_COMPAT_LAYER.en.md) - Detailed implementation instructions
- [Tim2Tox Architecture](ARCHITECTURE.en.md) - Overall architecture design
- [Development Guide](DEVELOPMENT_GUIDE.en.md) - Development Guide