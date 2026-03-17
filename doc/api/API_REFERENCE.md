# Tim2Tox API 参考
> 语言 / Language: [中文](API_REFERENCE.md) | [English](API_REFERENCE.en.md)

本文档为 Tim2Tox 的 API 索引入口，详细接口按模块拆分到子文档。

## 目录

- [V2TIM C++ API](API_REFERENCE_V2TIM.md) — C++ V2TIM 接口（与腾讯云 IM SDK 兼容）
- [C FFI 接口](API_REFERENCE_FFI.md) — C 语言绑定，供 Dart FFI 调用
- [Dart 包 API](API_REFERENCE_DART.md) — Dart 高级 API 与接口定义
- [数据类型](#数据类型) — 通用数据类型（本文档）
- [回调类型](#回调类型)
- [错误码](#错误码)
- [使用示例](#使用示例)
- [相关文档](#相关文档)

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

- [开发指南](../development/DEVELOPMENT_GUIDE.md) - 如何添加新功能和扩展
- [Tim2Tox 架构](../architecture/ARCHITECTURE.md) - 整体架构设计
- [Tim2Tox FFI 兼容层](../architecture/FFI_COMPAT_LAYER.md) - Dart* 函数兼容层说明
