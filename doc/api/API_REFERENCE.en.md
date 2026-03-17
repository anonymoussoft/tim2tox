# Tim2Tox API Reference
> Language: [Chinese](API_REFERENCE.md) | [English](API_REFERENCE.en.md)

This document is the index for Tim2Tox API reference. Detailed APIs are split into separate documents by module.

## Contents

- [V2TIM C++ API](API_REFERENCE_V2TIM.en.md) — C++ V2TIM interface (compatible with Tencent Cloud IM SDK)
- [C FFI Interface](API_REFERENCE_FFI.en.md) — C bindings for Dart FFI
- [Dart Package API](API_REFERENCE_DART.en.md) — Dart high-level API and interface definitions
- [Data Types](#data-types) — Common data types (this document)
- [Callback Types](#callback-types)
- [Error Codes](#error-codes)
- [Usage Examples](#usage-examples)
- [Related documents](#related-documents)

## Data Types

### V2TIMString

String type, compatible with C++ and Dart.

```cpp
class V2TIMString {
    const char* CString() const;
    // ...
};
```

### V2TIMBuffer

Binary data buffer.

```cpp
class V2TIMBuffer {
    const uint8_t* Data() const;
    size_t Size() const;
    // ...
};
```

### V2TIMMessage

Message object.

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

Friend information.

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

Group information.

```cpp
struct V2TIMGroupInfo {
    V2TIMString groupID;
    V2TIMString groupName;
    V2TIMString groupType; // "group" (new API) or "conference" (old API)
    V2TIMString notification; // Group announcement (only supported by Group type, corresponding to tox_group_topic)
    V2TIMString introduction;
    // ...
};
```

**V2TIMGroupMemberFullInfo**:
```cpp
struct V2TIMGroupMemberFullInfo {
    V2TIMString userID;
    V2TIMString nickName; // User nickname (obtained from friend information)
    V2TIMString nameCard; // Group nickname (obtained from tox_group_peer_get_name, only supported by Group type)
    uint32_t role; // Role: OWNER, ADMIN, MEMBER
    // ...
};
```

## Callback Types

### V2TIMCallback

Universal callback interface.

```cpp
class V2TIMCallback {
    virtual void OnSuccess() = 0;
    virtual void OnError(int error_code, const V2TIMString& error_message) = 0;
};
```

### V2TIMValueCallback

Callback interface with return value.

```cpp
template<class T>
class V2TIMValueCallback {
    virtual void OnSuccess(const T& value) = 0;
    virtual void OnError(int error_code, const V2TIMString& error_message) = 0;
};
```

### V2TIMSendCallback

Message sending callback interface.

```cpp
class V2TIMSendCallback {
    virtual void OnSuccess(const V2TIMMessage& message) = 0;
    virtual void OnError(int error_code, const V2TIMString& error_message) = 0;
    virtual void OnProgress(uint32_t progress) = 0;
};
```

## Error Codes

Common error codes are defined in `include/V2TIMErrorCode.h`:

- `ERR_SUCC` (0): Success
- `ERR_INVALID_PARAMETERS` (6017): Invalid parameter
- `ERR_SDK_NOT_INITIALIZED` (6018): SDK not initialized
- `ERR_USER_SIG_EXPIRED` (6206): User signature expired
- `ERR_SDK_COMM_API_CALL_FREQUENCY_LIMIT` (7008): API call frequency limit

## Usage Examples

### C++ Usage Example

```cpp
#include <V2TIMManager.h>

int main() {
    // Initialize SDK
    V2TIMSDKConfig config;
    config.logLevel = V2TIM_LOG_DEBUG;
    V2TIMManager::GetInstance()->InitSDK(123456, config);
    
    // Login
    V2TIMManager::GetInstance()->Login(
        V2TIMString("user123"),
        V2TIMString("userSig"),
        new V2TIMCallback(
            []() { /* Success */ },
            [](int code, const V2TIMString& msg) { /* failed */ }
        )
    );
    
    // Send message
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

### Dart Usage Example

```dart
import 'package:tim2tox_dart/tim2tox_dart.dart';

// Create service
final ffiService = FfiChatService(
  preferencesService: prefsAdapter,
  loggerService: loggerAdapter,
  bootstrapService: bootstrapAdapter,
);

// initialization
await ffiService.init();

// Login
await ffiService.login(userId: userID, userSig: userSig);

// send message
await ffiService.sendMessage(peerId, "Hello");

// Listen for messages
ffiService.messages.listen((message) {
  print('Received: ${message.text}');
});
```

## Related documents

- [Development Guide](../development/DEVELOPMENT_GUIDE.en.md) - How to add new features and extensions
- [Tim2Tox Architecture](../architecture/ARCHITECTURE.en.md) - Overall architecture design
- [Tim2Tox FFI compatibility layer](../architecture/FFI_COMPAT_LAYER.en.md) - Dart* function compatibility layer description
