# Tim2Tox 二进制替换
> 语言 / Language: [中文](BINARY_REPLACEMENT.md) | [English](BINARY_REPLACEMENT.en.md)


本文档详细说明如何使用底层 Tox 二进制实现替换 TIMSDK 的二进制，实现零 Dart 代码修改的集成方案。

## 概述

二进制替换方案（BINARY REPLACEMENT MODE）是一种通过替换动态库文件来实现后端切换的方案，使得 Dart 层代码完全不需要修改，就能从腾讯云 IM SDK 切换到 Tox P2P 协议。

**当前使用状态**: ✅ **toxee 使用混合模式（Binary Replacement + Platform 接口）**

**配置方式**:
- 在 `main()` 最早期调用 `setNativeLibraryName('tim2tox_ffi')` 配置库名
- 可选：设置 `TencentCloudChatSdkPlatform.instance = Tim2ToxSdkPlatform(...)` 启用高级功能

## 核心原理

### 1. 动态库替换

**关键文件**: `tencent_cloud_chat_sdk-8.7.7201/lib/native_im/bindings/native_library_manager.dart`

在 `NativeLibraryManager` 中，通过 `setNativeLibraryName()` 配置动态库名称（默认为原始 SDK 的 `dart_native_imsdk`）：

```dart
// 默认值为原始 SDK，调用方在 main() 最早期覆盖
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
  // ... 其他平台的处理
}();
```

**使用方式**（在 `main()` 最早期、任何 SDK 调用之前）：
```dart
import 'package:tencent_cloud_chat_sdk/native_im/bindings/native_library_manager.dart';
setNativeLibraryName('tim2tox_ffi');
```

**重要**: `_dylib` 是顶层 `final` 变量，在首次访问时惰性初始化。`setNativeLibraryName()` 必须在任何 `NativeLibraryManager.bindings` 使用之前调用，否则会加载默认的 `dart_native_imsdk`。

**编译产物**:
- macOS: `libtim2tox_ffi.dylib`
- Linux: `libtim2tox_ffi.so`
- iOS: `tim2tox_ffi.framework` 或 `libtim2tox_ffi.dylib`
- Android: `libtim2tox_ffi.so`
- Windows: `tim2tox_ffi.dll`

### 2. 函数签名完全匹配

**关键文件**: `tim2tox/ffi/dart_compat_layer.cpp`

所有 Dart* 函数的签名必须与 `native_imsdk_bindings_generated.dart` 中定义的完全一致：

```cpp
// 示例：DartInitSDK 函数
extern "C" {
    int DartInitSDK(const char* json_init_param, void* user_data) {
        // 1. 解析 JSON 参数
        std::string json_str(json_init_param);
        std::string sdk_app_id_str = ExtractJsonValue(json_str, "sdk_config_sdk_app_id");
        // ...
        
        // 2. 调用 V2TIM SDK API
        V2TIMManager::GetInstance()->InitSDK(sdkAppID, config);
        
        // 3. 通过回调返回结果
        SendApiCallbackResult(user_data, 0, "OK");
        return 0;
    }
}
```

**函数签名来源**: `native_imsdk_bindings_generated.dart` 是通过 `ffigen` 从原生 SDK 的头文件自动生成的，确保签名完全匹配。

### 3. FFI 动态符号查找

**关键文件**: `tencent_cloud_chat_sdk-8.7.7201/lib/native_im/bindings/native_imsdk_bindings_generated.dart`

Dart 层通过 FFI 动态查找符号：

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

当 `_dylib` 指向 `libtim2tox_ffi.dylib` 时，`_lookup('DartInitSDK')` 会在 `libtim2tox_ffi.dylib` 中查找符号，而不是原生的 `dart_native_imsdk`。

### 4. 回调机制

**关键文件**: `tim2tox/ffi/callback_bridge.cpp`

C++ 层通过 `Dart_PostCObject_DL` 将事件发送回 Dart 层：

```cpp
void SendCallbackToDart(const char* callback_type, const std::string& json_data, void* user_data) {
    // 1. 构建 JSON 消息
    std::string message = json_data; // 已包含 "callback" 字段
    
    // 2. 创建 Dart_CObject
    Dart_CObject cobj;
    cobj.type = Dart_CObject_kString;
    char* message_cstr = static_cast<char*>(malloc(message.length() + 1));
    std::memcpy(message_cstr, message.c_str(), message.length());
    cobj.value.as_string = message_cstr;
    
    // 3. 发送到 Dart 层
    Dart_PostCObject_DL(g_dart_port, &cobj);
}
```

**初始化流程**:
1. Dart 层调用 `DartInitDartApiDL(data)` 初始化 Dart API
2. Dart 层调用 `DartRegisterSendPort(port)` 注册 SendPort
3. C++ 层存储端口号，用于后续发送回调

## 完整调用流程

### SDK 初始化流程

```
Dart 层 (TIMManager.initSDK)
  ↓
NativeLibraryManager.bindings.DartInitSDK(...)
  ↓
FFI 动态查找符号 'DartInitSDK' (在 libtim2tox_ffi.dylib 中)
  ↓
C++ 层 (dart_compat_layer.cpp::DartInitSDK)
  ↓
解析 JSON 参数 (json_parser.cpp)
  ↓
V2TIMManager::GetInstance()->InitSDK(...)
  ↓
ToxManager::getInstance().init(...)
  ↓
tox_new() (c-toxcore)
  ↓
回调: SendApiCallbackResult(user_data, 0, "OK")
  ↓
Dart 层 (ReceivePort 接收回调)
  ↓
NativeLibraryManager._handleNativeMessage()
  ↓
业务代码处理结果
```

### 消息发送流程

```
Dart 层 (TIMMessageManager.sendMessage)
  ↓
NativeLibraryManager.bindings.DartSendMessage(...)
  ↓
C++ 层 (dart_compat_layer.cpp::DartSendMessage)
  ↓
V2TIMMessageManagerImpl::SendMessage(...)
  ↓
ToxManager::SendMessage(...)
  ↓
tox_friend_send_message() (c-toxcore)
  ↓
P2P 网络传输
```

### 消息接收流程

```
P2P 网络接收
  ↓
tox_friend_message() 回调 (c-toxcore)
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
Dart 层 (ReceivePort 接收回调)
  ↓
NativeLibraryManager._handleNativeMessage()
  ↓
NativeLibraryManager._handleGlobalCallback()
  ↓
业务代码处理消息
```

## 关键组件

### 1. dart_compat_layer.cpp/h

**位置**: `tim2tox/ffi/dart_compat_layer.cpp`

**职责**: 实现所有 Dart* 函数，提供与原生 SDK 完全兼容的接口。

**主要函数**:
- `DartInitSDK`: SDK 初始化
- `DartLogin`: 用户登录
- `DartSendMessage`: 发送消息
- `DartGetConversationList`: 获取会话列表
- `DartGetFriendList`: 获取好友列表
- 等等...

**实现模式**:
```cpp
int DartXXX(const char* json_param, void* user_data) {
    // 1. 解析 JSON 参数
    std::string json_str(json_param);
    std::string field1 = ExtractJsonValue(json_str, "field1");
    
    // 2. 调用 V2TIM SDK API
    V2TIMManager::GetInstance()->GetXXXManager()->XXX(...);
    
    // 3. 通过回调返回结果（异步）
    SendApiCallbackResult(user_data, code, desc);
    return 0;
}
```

### 2. callback_bridge.cpp/h

**位置**: `tim2tox/ffi/callback_bridge.cpp`

**职责**: 实现回调桥接机制，将 C++ 事件发送到 Dart 层。

**主要函数**:
- `DartInitDartApiDL`: 初始化 Dart API
- `DartRegisterSendPort(int64_t send_port)`: 注册 Dart SendPort（签名需与 native_imsdk_bindings_generated.dart 一致）
- `SendCallbackToDart`: 发送回调消息到 Dart 层

### 3. json_parser.cpp/h

**位置**: `tim2tox/ffi/json_parser.cpp`

**职责**: 实现 JSON 消息构建和解析工具。

**主要函数**:
- `BuildGlobalCallbackJson`: 构建 globalCallback JSON 消息
- `BuildApiCallbackJson`: 构建 apiCallback JSON 消息
- `ExtractJsonValue`: 提取 JSON 值
- `ParseJsonString`: 解析 JSON 字符串

### 4. Listener 实现

**位置**: `tim2tox/ffi/dart_compat_layer.cpp`

**职责**: 实现 V2TIM Listener 接口，将 Tox 事件转换为 JSON 消息。

**主要 Listener**:
- `DartSDKListenerImpl`: SDK 事件监听（连接状态、用户状态等）
- `DartAdvancedMsgListenerImpl`: 消息事件监听（新消息、消息修改等）
- `DartFriendshipListenerImpl`: 好友事件监听（好友添加、删除等）
- `DartConversationListenerImpl`: 会话事件监听（会话更新等）
- `DartGroupListenerImpl`: 群组事件监听（群组提示等）
- `DartSignalingListenerImpl`: 信令事件监听（音视频邀请等）
- `DartCommunityListenerImpl`: 社区事件监听（话题创建等）

## 与 Platform 接口方案的区别

### 方案对比

| 特性 | 二进制替换方案 | Platform 接口方案 | 混合模式（当前使用） |
|------|-------------|-----------------|-------------------|
| **Dart 代码修改** | 仅 `setNativeLibraryName` | 需要设置 Platform | 两者都需要 |
| **功能丰富度** | 基础功能 | 高级功能 | 完整功能 |
| **自定义 callback** | 通过 `customCallbackHandler` | 直接在 Platform 实现 | `customCallbackHandler` 注册到 Platform |
| **适用场景** | 快速集成 | 需要高级功能 | 生产环境 |

### 混合模式（toxee 当前使用）

toxee 同时使用二进制替换和 Platform 接口：

1. `setNativeLibraryName('tim2tox_ffi')` — 加载 `libtim2tox_ffi.dylib`
2. `TencentCloudChatSdkPlatform.instance = Tim2ToxSdkPlatform(...)` — 启用高级功能
3. `Tim2ToxSdkPlatform` 构造函数注册 `NativeLibraryManager.customCallbackHandler` — 接管自定义 callback

### customCallbackHandler 机制

SDK 中 `NativeLibraryManager._handleNativeCallback` 的 switch 默认分支委托给 `customCallbackHandler`：

```dart
// NativeLibraryManager 类内
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
```

Tim2ToxSdkPlatform 在构造时注册 handler，处理 3 个自定义 callback：
- `clearHistoryMessage` — 清除 C2C/群组聊天历史
- `groupQuitNotification` — 群退出后清理状态
- `groupChatIdStored` — 存储群 chat_id 映射

### Platform 接口方案

**调用路径**：
```
UIKit SDK
  ↓
TencentCloudChatSdkPlatform.instance (Tim2ToxSdkPlatform)
  ↓
FfiChatService (高级服务层)
  ↓
Tim2ToxFfi (FFI 绑定)
  ↓
tim2tox_ffi_* (C FFI 接口)
  ↓
V2TIM*Manager (C++ API 实现)
  ↓
ToxManager (Tox 核心)
  ↓
c-toxcore (P2P 通信)
```

**特点**:
- 需要设置 `TencentCloudChatSdkPlatform.instance = Tim2ToxSdkPlatform(...)`
- 需要修改 Dart 层代码
- 使用高级服务层（FfiChatService）提供更丰富的功能
- 支持消息历史管理、事件轮询、状态管理等高级功能

**关键文件**:
- `tim2tox/dart/lib/sdk/tim2tox_sdk_platform.dart` - Platform 接口实现
- `tim2tox/dart/lib/service/ffi_chat_service.dart` - 高级服务层
- `tim2tox/dart/lib/ffi/tim2tox_ffi.dart` - FFI 绑定

### 纯二进制替换方案

**调用路径**：
```
UIKit SDK
  ↓
TIMManager.instance
  ↓
NativeLibraryManager (原生 SDK 的调用方式)
  ↓
bindings.DartXXX(...) (FFI 动态查找符号)
  ↓
libtim2tox_ffi.dylib (替换后的动态库)
  ↓
dart_compat_layer.cpp (Dart* 函数实现)
  ↓
V2TIM*Manager (C++ API 实现)
  ↓
ToxManager (Tox 核心)
  ↓
c-toxcore (P2P 通信)
```

**特点**:
- **零 Dart 代码修改**: 不需要设置 `TencentCloudChatSdkPlatform.instance`
- **完全兼容**: 函数签名和回调格式完全匹配原生 SDK
- **只需替换动态库**: 将 `dart_native_imsdk` 替换为 `libtim2tox_ffi`
- **适用场景**: 快速验证、最小化接入、隔离调试

**关键文件**:
- `tencent_cloud_chat_sdk-8.7.7201/lib/native_im/bindings/native_library_manager.dart` - 动态库加载
- `tim2tox/ffi/dart_compat_layer.cpp` - Dart* 函数实现
- `tim2tox/ffi/callback_bridge.cpp` - 回调桥接机制

## 部署方式

### 1. 编译动态库

**位置**: `tim2tox/ffi/CMakeLists.txt`

```bash
cd tim2tox
mkdir -p build && cd build
cmake ..
make tim2tox_ffi
```

**输出**:
- macOS: `build/ffi/libtim2tox_ffi.dylib`
- Linux: `build/ffi/libtim2tox_ffi.so`

### 2. 打包到应用

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

### 3. 依赖库处理

**libsodium** (Tox 的加密库依赖):
- macOS: 使用 `install_name_tool` 修改动态库的依赖路径
- Linux: 确保系统已安装 `libsodium` 或打包到应用

## 优势

1. **最小 Dart 代码修改**: 仅需在 `main()` 中添加一行 `setNativeLibraryName('tim2tox_ffi')`
2. **SDK 无专有代码**: SDK 中只有通用扩展点（`setNativeLibraryName`、`customCallbackHandler`、`isCustomPlatform`），无 tim2tox 专有逻辑
3. **易于 SDK 升级**: SDK 升级时只需合并通用扩展点和 bug 修复（内存管理、空安全），不涉及 tim2tox 专有代码
4. **完全兼容**: 函数签名和回调格式完全匹配原生 SDK
5. **可测试**: 通过 `tim2tox_ffi_inject_callback` FFI 函数可注入 callback 进行单元测试

## 限制

1. **函数签名必须完全匹配**: 任何签名不匹配都会导致运行时错误
2. **JSON 格式必须匹配**: 参数和回调的 JSON 格式必须与原生 SDK 完全一致
3. **回调机制依赖**: 依赖 Dart API DL 的回调机制，需要正确初始化

## 已修复问题

本节记录在二进制替换方案开发和测试过程中发现并已修复的问题。

### 1. 会话置顶功能流程修复

**问题描述**：
- `OnConversationChanged` 事件中 `conv_event` 字段未正确设置
- 导致 Dart 层无法正确识别会话更新事件

**修复方案**：
- 在 `DartConversationListenerImpl::OnConversationChanged` 中正确设置 `conv_event = "2"`（conversationEventUpdate）
- 确保 `ConversationVectorToJson` 包含 `conv_is_pinned` 字段
- 验证完整的事件流程：C++ 层 → globalCallback → Dart 层 → UIKit SDK

**已修复的代码位置**：
- `tim2tox/ffi/dart_compat_listeners.cpp:DartConversationListenerImpl::OnConversationChanged()`
- `tim2tox/ffi/dart_compat_utils.cpp:ConversationVectorToJson()`

**验证结果**：
- ✅ 会话置顶/取消置顶操作正确触发 `OnConversationChanged` 事件
- ✅ Dart 层正确解析 `conv_event = "2"` 并路由到 `onConversationChanged`
- ✅ UI 正确更新会话列表的置顶状态

### 2. 群组ID生成逻辑修复

**问题描述**：
- 使用 `conference_number` 直接生成群组 ID（`tox_%u`），导致 ID 重用冲突
- 当旧的群组被删除后，新的群组可能重用相同的 ID

**修复方案**：
- 使用全局计数器 `next_group_id_counter_` 生成唯一 ID（`tox_%llu`）
- 检查冲突并确保唯一性
- 在退出/删除群组时正确清理映射关系，防止 ID 重用

**已修复的代码位置**：
- `tim2tox/source/V2TIMManagerImpl.cpp:CreateGroup()`
- `tim2tox/source/V2TIMManagerImpl.cpp:QuitGroup()`
- `tim2tox/source/V2TIMManagerImpl.cpp:DismissGroup()`

**验证结果**：
- ✅ 群组ID生成唯一，避免重用历史ID
- ✅ 退出/删除群组时正确清理映射关系

### 3. GetJoinedGroupList 和 SearchGroups 修复

**问题描述**：
- `GetJoinedGroupList` 和 `SearchGroups` 直接生成群组ID，而不是使用已建立的映射关系
- 导致返回的群组ID与创建时的不一致

**修复方案**：
- 优先从 `V2TIMManagerImpl` 的映射中获取群组ID
- 如果映射中不存在，才尝试通过其他方式获取
- 确保返回的群组ID与创建时的一致

**已修复的代码位置**：
- `tim2tox/source/V2TIMGroupManagerImpl.cpp:GetJoinedGroupList()`
- `tim2tox/source/V2TIMGroupManagerImpl.cpp:SearchGroups()`

**验证结果**：
- ✅ `GetJoinedGroupList` 和 `SearchGroups` 正确使用映射关系
- ✅ 返回的群组ID与创建时的一致

## 调试

### 检查符号导出

**macOS**:
```bash
nm -D libtim2tox_ffi.dylib | grep Dart
```

**Linux**:
```bash
nm -D libtim2tox_ffi.so | grep Dart
```

### 检查动态库依赖

**macOS**:
```bash
otool -L libtim2tox_ffi.dylib
```

**Linux**:
```bash
ldd libtim2tox_ffi.so
```

### 日志输出

C++ 层使用 `fprintf(stdout, ...)` 和 `fprintf(stderr, ...)` 输出日志，可以在 Flutter 控制台看到。

## 相关文档

- [Tim2Tox FFI 兼容层](FFI_COMPAT_LAYER.md) - 详细的实现说明
- [Tim2Tox 架构](ARCHITECTURE.md) - 整体架构设计
- [开发指南](../development/DEVELOPMENT_GUIDE.md) - 开发指南
