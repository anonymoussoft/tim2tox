# Tim2Tox 架构
> 语言 / Language: [中文](ARCHITECTURE.md) | [English](ARCHITECTURE.en.md)


本文档描述 Tim2Tox 的整体架构设计、模块划分、数据流图和关键设计决策。

## 目录

- [整体架构](#整体架构)
- [模块划分](#模块划分)
- [数据流](#数据流)
- [关键设计决策](#关键设计决策)
- [接口抽象](#接口抽象)
- [依赖注入](#依赖注入)

## 整体架构

Tim2Tox 采用分层架构，从底层到顶层分为：

```
┌─────────────────────────────────────────────────────────┐
│              Flutter/Dart 应用层                        │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐            │
│  │   UIKit  │  │  Client  │  │  Custom  │            │
│  │   SDK    │  │   Code   │  │   Code   │            │
│  └────┬─────┘  └────┬─────┘  └────┬─────┘            │
└───────┼──────────────┼─────────────┼────────────────────┘
        │              │             │
        └──────────────┴─────────────┘
                       │
        ┌──────────────▼──────────────┐
        │    Dart SDK Platform 层      │
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
        │      C FFI 接口层            │
        │  ┌────────┐  ┌──────────┐  │
        │  │tim2tox │  │   Dart*  │  │
        │  │  FFI   │  │ Compat   │  │
        │  └───┬────┘  └────┬─────┘  │
        └──────┼─────────────┼────────┘
               │             │
        ┌──────▼─────────────▼──────┐
        │    V2TIM API 实现层         │
        │  ┌────────┐  ┌──────────┐ │
        │  │Manager│  │  Manager │ │
        │  │ Impl  │  │   Impl   │ │
        │  └───┬───┘  └────┬─────┘ │
        └──────┼───────────┼────────┘
               │           │
        ┌──────▼───────────▼──────┐
        │     Tox 核心层            │
        │  ┌────────┐  ┌────────┐ │
        │  │ Tox    │  │ ToxAV  │ │
        │  │Manager │  │Manager │ │
        │  └───┬────┘  └────┬───┘ │
        └──────┼────────────┼──────┘
               │            │
        ┌──────▼────────────▼──────┐
        │      c-toxcore            │
        │   (P2P 通信库)            │
        └───────────────────────────┘
```

## 模块划分

### 1. V2TIM API 层 (`include/`, `source/`)

**职责**: 实现与腾讯云 IM SDK 兼容的 V2TIM API

**主要模块**:
- `V2TIMManager`: 核心管理器，提供 SDK 初始化、登录、获取其他管理器
- `V2TIMMessageManager`: 消息管理，创建、发送、接收消息
- `V2TIMFriendshipManager`: 好友管理，添加、删除、查询好友
- `V2TIMGroupManager`: 群组管理，创建、加入、管理群组
- `V2TIMConversationManager`: 会话管理，查询、删除会话
- `V2TIMSignalingManager`: 信令管理，音视频通话邀请
- `V2TIMCommunityManager`: 社区管理，话题和权限组管理

**设计原则**:
- 完全兼容腾讯云 IM SDK 的 V2TIM API
- 使用回调机制处理异步操作
- 所有操作都是线程安全的

### 2. Tox 核心层 (`source/ToxManager.*`)

**职责**: 管理 Tox 实例和生命周期，处理底层 P2P 通信

**主要组件**:
- `ToxManager`: 管理 Tox 实例，处理好友、消息、群组等核心功能
- `ToxAVManager`: 管理音视频通话（可选）
- `IrcClientManager`: IRC 通道桥接管理（可选）

**设计原则**:
- 单一 Tox 实例管理
- 事件驱动的架构
- 自动重连和错误恢复

#### 群聊实现

Tim2Tox 同时支持两种 Tox 群聊 API：

**Group API（新 API）**：
- 使用 `tox_group_new` 创建
- 支持 `chat_id` 持久化（32 字节）
- 可以通过 `chat_id` 重新加入
- 功能完整，支持群公告、成员管理等
- **推荐使用**

**Conference API（旧 API）**：
- 使用 `tox_conference_new` 创建
- 不支持 `chat_id`，依赖 savedata 恢复
- 只能通过好友邀请加入
- 功能相对简单
- **仅用于兼容性**

**映射关系管理**：
- `group_id` ↔ `group_number` ↔ `chat_id` 之间的双向映射
- 存储在内存中，重启后需要恢复
- 通过 `chat_id` 可以重建映射关系

**关键代码位置**：
- `tim2tox/source/V2TIMGroupManagerImpl.cpp` - 群组管理实现
- `tim2tox/source/V2TIMManagerImpl.cpp` - 群组恢复机制
- `tim2tox/source/ToxManager.cpp` - Tox 群组操作封装

### 3. FFI 接口层 (`ffi/`)

**职责**: 提供 C 接口，供 Dart FFI 调用

**主要组件**:
- `tim2tox_ffi.h/cpp`: 主要 FFI 接口，提供高级 API
- `dart_compat_layer.h/cpp`: Dart* 函数兼容层主入口（已模块化）
- `dart_compat_internal.h`: 共享声明和前置声明
- **模块化实现**（13个模块文件）:
  - `dart_compat_utils.cpp`: 工具函数和全局变量
  - `dart_compat_listeners.cpp`: 监听器实现和回调注册
  - `dart_compat_callbacks.cpp`: 回调类实现
  - `dart_compat_sdk.cpp`: SDK初始化和认证
  - `dart_compat_message.cpp`: 消息相关功能
  - `dart_compat_friendship.cpp`: 好友相关功能
  - `dart_compat_conversation.cpp`: 会话相关功能
  - `dart_compat_group.cpp`: 群组相关功能
  - `dart_compat_user.cpp`: 用户相关功能
  - `dart_compat_signaling.cpp`: 信令相关功能
  - `dart_compat_community.cpp`: 社区相关功能
  - `dart_compat_other.cpp`: 其他杂项功能
- `callback_bridge.h/cpp`: 回调桥接机制，将 C++ 事件发送到 Dart
- `json_parser.h/cpp`: JSON 消息构建和解析工具

**设计原则**:
- C 接口，无 C++ 依赖
- 线程安全的回调机制
- JSON 格式的消息传递
- **模块化设计**: 代码按功能拆分为独立模块，提高可维护性

### 4. Dart 绑定层 (`dart/lib/`)

**职责**: 提供 Flutter/Dart 绑定和高级 API

**主要组件**:
- `ffi/tim2tox_ffi.dart`: 底层 FFI 绑定，直接映射 C 函数
- `service/ffi_chat_service.dart`: 高级服务层，管理消息历史、轮询、状态
- `sdk/tim2tox_sdk_platform.dart`: SDK Platform 实现，路由 UIKit SDK 调用
- `service/toxav_service.dart`: ToxAV 管理与实例路由
- `service/call_bridge_service.dart`: signaling 与 ToxAV 的桥接
- `interfaces/`: 抽象接口定义，支持依赖注入

**设计原则**:
- 清晰的层次分离
- 异步操作使用 Future/Stream
- 接口抽象，支持依赖注入

## 集成方案

Tim2Tox 能力上支持三种接入形态：

- **纯二进制替换**：只替换动态库，不注册 Platform
- **Platform 接口方案**：只通过 `Tim2ToxSdkPlatform` 路由
- **混合架构**：同时保留二进制替换和 Platform 路径

其中，**toxee 当前使用的是混合架构**。

### 方案一：纯二进制替换方案

**特点**：
- ✅ **零 Dart 代码修改**：不需要设置 `TencentCloudChatSdkPlatform.instance`
- ✅ **完全兼容原生 SDK**：使用 `TIMManager.instance` → `NativeLibraryManager` → Dart* 函数
- ✅ **只需替换动态库**：将 `dart_native_imsdk` 替换为 `libtim2tox_ffi`

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

**关键组件**：
- `tim2tox/ffi/dart_compat_layer.cpp` - 实现所有 Dart* 函数
- `tim2tox/ffi/callback_bridge.cpp` - 回调桥接机制
- `tencent_cloud_chat_sdk-8.7.7201/lib/native_im/bindings/native_library_manager.dart` - 动态库加载

**详细文档**：参见 [Tim2Tox 二进制替换](BINARY_REPLACEMENT.md)

### 方案二：Platform 接口方案（备选）

**特点**：
- 需要设置 `TencentCloudChatSdkPlatform.instance = Tim2ToxSdkPlatform(...)`
- 使用高级服务层（FfiChatService）提供更丰富的功能
- 需要修改 Dart 层代码

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

**关键组件**：
- `tim2tox/dart/lib/sdk/tim2tox_sdk_platform.dart` - Platform 接口实现
- `tim2tox/dart/lib/service/ffi_chat_service.dart` - 高级服务层
- `tim2tox/dart/lib/ffi/tim2tox_ffi.dart` - FFI 绑定

### 方案三：混合架构（toxee 当前使用）

**特点**：
- `main()` 中先执行 `setNativeLibraryName('tim2tox_ffi')`
- 客户端在 HomePage 中注册 `Tim2ToxSdkPlatform`
- NativeLibraryManager 继续负责大部分二进制替换调用
- Platform 路径负责历史消息、自定义 callback、通话与部分高级能力

**额外组件**：
- `sdk/tim2tox_sdk_platform.dart` 中的 `customCallbackHandler`
- `service/toxav_service.dart`、`service/call_bridge_service.dart`
- `service/tuicallkit_adapter.dart`、`service/tuicallkit_tuicore_integration.dart`

### 方案对比

| 特性 | 纯二进制替换 | Platform 接口方案 | 混合架构 |
|------|----------------|-------------------|----------|
| Dart 代码修改 | ❌ 不需要 | ✅ 需要 | ✅ 需要 |
| 部署复杂度 | ✅ 简单 | ⚠️ 中等 | ⚠️ 中等 |
| 功能丰富度 | ⚠️ 基础功能 | ✅ 高级功能 | ✅ 完整 |
| 维护成本 | ✅ 低 | ⚠️ 中等 | ⚠️ 中高 |
| 兼容性 | ✅ 完全兼容原生 SDK | ⚠️ 需要适配 UIKit SDK | ✅ 当前客户端已验证 |
| 适用场景 | 快速验证、最小接入 | 独立 SDK 封装 | 客户端生产接入 |

## 数据流

### SDK 调用路径（二进制替换方案）

```
UIKit SDK Calls
  ↓
TIMManager.instance
  ↓
NativeLibraryManager.bindings.DartXXX(...)
  ↓
FFI 动态查找符号 (在 libtim2tox_ffi 中)
  ↓
dart_compat_layer.cpp (Dart* 函数实现)
  ↓
V2TIM*Manager (C++ API 实现)
  ↓
ToxManager (Tox 核心)
  ↓
c-toxcore (P2P 通信)
```

### SDK 调用路径（Platform 接口方案）

```
UIKit SDK Calls
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

### 消息发送路径

```
用户输入消息
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
P2P 网络传输
```

### 消息接收路径

```
P2P 网络接收
  ↓
tox_friend_message() (c-toxcore 回调)
  ↓
ToxManager::OnFriendMessage()
  ↓
V2TIMMessageManagerImpl::OnRecvNewMessage()
  ↓
Listener 回调 (DartAdvancedMsgListenerImpl)
  ↓
SendCallbackToDart() (JSON 消息)
  ↓
Dart ReceivePort
  ↓
NativeLibraryManager._handleNativeMessage()
  ↓
Tim2ToxSdkPlatform 事件分发
  ↓
UIKit Listeners
  ↓
UI 更新
```

### 回调机制

Tim2Tox 通过 `SendCallbackToDart` 函数实现 C++ 到 Dart 层的通知机制。

#### 初始化流程

1. **Dart 层注册 SendPort**：
   ```dart
   NativeLibraryManager.registerPort();
   // 内部调用：bindings.DartRegisterSendPort(_receivePort.sendPort.nativePort);
   ```

2. **C++ 层存储 SendPort**：
   ```cpp
   void DartRegisterSendPort(int64_t send_port) {
       g_dart_port = static_cast<Dart_Port>(send_port);
   }
   ```

3. **C++ 层发送消息**：
   ```cpp
   SendCallbackToDart("callback_type", json_data, user_data);
   ```

4. **Dart 层接收消息**：
   ```dart
   _receivePort.listen((dynamic message) {
       _handleNativeMessage(message);
   });
   ```

#### 回调类型

**1. `apiCallback` - API 调用结果回调**

用于返回异步 API 调用的结果：

```json
{
  "callback": "apiCallback",
  "user_data": "unique_id",
  "code": 0,
  "desc": "success",
  "json_param": "{...}"  // 可选
}
```

**2. `globalCallback` - 全局事件回调**

用于通知各种全局事件（网络状态、消息接收、好友变化等）：

```json
{
  "callback": "globalCallback",
  "callbackType": 7,  // GlobalCallbackType 枚举值
  "json_message": "{...}",  // 或其他字段
  "user_data": "..."
}
```

**3. `groupQuitNotification` - 群组退出通知**

用于通知 Dart 层清理群组状态：

```json
{
  "callback": "groupQuitNotification",
  "group_id": "tox_1",
  "user_data": "..."
}
```

#### 完整回调流程

```
C++ 层事件
  ↓
Listener 实现 (Dart*ListenerImpl)
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
Dart 层回调处理
```

**关键代码位置**：
- `tim2tox/ffi/callback_bridge.cpp:SendCallbackToDart()`
- `tim2tox/ffi/json_parser.cpp:BuildGlobalCallbackJson()`
- `tim2tox/ffi/json_parser.cpp:BuildApiCallbackJson()`

## 关键设计决策

### 1. 接口抽象和依赖注入

**决策**: Tim2Tox 通过接口注入客户端依赖，而不是直接依赖客户端代码

**原因**:
- Tim2Tox 完全独立，可被任何 Flutter 客户端复用
- 客户端可以自由选择实现方式（SharedPreferences、自定义存储等）
- 便于测试和模拟

**实现**:
- `ExtendedPreferencesService`: 偏好设置接口
- `LoggerService`: 日志接口
- `BootstrapService`: Bootstrap 节点接口
- `EventBusProvider`: 事件总线接口
- `ConversationManagerProvider`: 会话管理器接口

### 2. 双重数据路径

**决策**: 同时支持 SDK Events 和 Data Streams 两种数据路径

**原因**:
- SDK Events: 兼容 UIKit SDK 的监听器机制
- Data Streams: 提供更灵活的数据流处理

**实现**:
- SDK Events: 通过 `Tim2ToxSdkPlatform` 路由到 UIKit SDK Listeners
- Data Streams: 通过 `FfiChatService.messages` Stream 提供消息流

### 3. 消息 ID 管理

**决策**: 使用 `timestamp_userID` 格式作为消息 ID

**原因**:
- 确保消息 ID 的唯一性
- 便于排序和查询
- 兼容 UIKit SDK 的期望

**实现**:
- 消息发送时生成: `"${timestamp}_${userID}"`
- 消息接收时解析时间戳和用户 ID

### 4. 失败消息处理

**决策**: 实现完整的失败消息检测和持久化机制

**原因**:
- P2P 网络可能不稳定
- 需要检测离线状态和超时
- 需要持久化失败消息以便恢复

**实现**:
- 离线检测: 立即检测联系人是否在线
- 超时检测: 文本消息 5 秒，文件消息根据大小动态计算
- 持久化: 使用 SharedPreferences 存储失败消息
- 恢复: 客户端重启后自动恢复失败消息状态

### 5. 二进制替换方案

**决策**: 实现 Dart* 函数兼容层，支持二进制替换

**原因**:
- 最小化 Dart 层代码修改
- 完全保留 NativeLibraryManager 的调用方式
- 支持逐步迁移

**实现**:
- `dart_compat_layer.cpp`: 导出所有 Dart* 函数
- `callback_bridge.cpp`: 实现回调桥接机制
- `json_parser.cpp`: 实现 JSON 消息构建和解析

### 6. 异步操作处理

**决策**: 所有耗时操作使用异步回调

**原因**:
- 避免阻塞主线程
- 提供更好的用户体验
- 兼容 UIKit SDK 的异步模型

**实现**:
- C++ 层: 使用 `V2TIMCallback` 和 `V2TIMValueCallback`
- Dart 层: 使用 `Future` 和 `Stream`
- FFI 层: 通过 JSON 消息传递结果

## 接口抽象

### 必需接口

客户端必须实现以下接口：

#### ExtendedPreferencesService

偏好设置服务，用于持久化数据。

```dart
abstract class ExtendedPreferencesService {
  Future<String?> getString(String key);
  Future<bool> setString(String key, String value);
  // ... 其他方法
}
```

#### LoggerService

日志服务，用于输出日志。

```dart
abstract class LoggerService {
  void log(String message);
  void logError(String message, Object error, StackTrace stack);
  void logWarning(String message);
  void logDebug(String message);
}
```

#### BootstrapService

Bootstrap 节点服务，用于 Tox 网络连接。

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

### 可选接口

客户端可以选择实现以下接口以启用高级功能：

#### EventBusProvider

事件总线提供者，用于组件间通信。

```dart
abstract class EventBusProvider {
  EventBus get eventBus;
}
```

#### ConversationManagerProvider

会话管理器提供者，用于会话管理。

```dart
abstract class ConversationManagerProvider {
  ConversationManager get conversationManager;
}
```

## 依赖注入

### 初始化流程

```dart
// 1. 创建接口适配器
final prefsAdapter = SharedPreferencesAdapter(await SharedPreferences.getInstance());
final loggerAdapter = AppLoggerAdapter();
final bootstrapAdapter = BootstrapNodesAdapter(await SharedPreferences.getInstance());

// 2. 创建服务
final ffiService = FfiChatService(
  preferencesService: prefsAdapter,
  loggerService: loggerAdapter,
  bootstrapService: bootstrapAdapter,
);

// 3. 初始化服务
await ffiService.init();

// 4. 设置 SDK Platform
TencentCloudChatSdkPlatform.instance = Tim2ToxSdkPlatform(
  ffiService: ffiService,
  eventBusProvider: eventBusAdapter,        // 可选
  conversationManagerProvider: convAdapter,  // 可选
);
```

### 适配器模式

客户端通过适配器将 Tim2Tox 的抽象接口映射到具体实现：

```dart
class SharedPreferencesAdapter implements ExtendedPreferencesService {
  final SharedPreferences _prefs;
  SharedPreferencesAdapter(this._prefs);
  
  @override
  Future<String?> getString(String key) async => _prefs.getString(key);
  
  // ... 实现其他方法
}
```

## 线程模型

### C++ 层

- **主线程**: 所有 V2TIM API 调用应在主线程进行
- **Tox 线程**: Tox 事件回调在 Tox 线程执行
- **回调线程**: 通过 `Dart_PostCObject_DL` 发送到 Dart 线程

### Dart 层

- **UI 线程**: 所有 UI 操作和 SDK 调用在 UI 线程
- **Isolate**: 可选，用于耗时操作

## 内存管理

### C++ 层

- 使用智能指针 (`std::shared_ptr`, `std::unique_ptr`)
- 字符串使用 `V2TIMString` 管理生命周期
- 回调对象由调用者管理生命周期

### Dart 层

- 使用 Dart 的垃圾回收机制
- FFI 字符串使用 `toNativeUtf8()` 和 `malloc.free()`
- Stream 使用 `StreamController` 管理

## 错误处理

### C++ 层

- 使用错误码和错误消息
- 通过回调返回错误信息
- 记录详细日志

### Dart 层

- 使用 `Future` 的异常处理
- 通过 `V2TimCallback` 返回错误码和描述
- 提供用户友好的错误提示

## 性能优化

### 消息处理

- 批量处理消息
- 使用消息队列避免阻塞
- 异步处理耗时操作

### 网络优化

- 连接池管理
- 自动重连机制
- 网络状态监控

### 内存优化

- 对象池复用
- 及时释放不用的资源
- 避免内存泄漏

## 相关文档

- [API 参考](API_REFERENCE.md) - 完整 API 文档
- [开发指南](DEVELOPMENT_GUIDE.md) - 开发指南
- [Tim2Tox FFI 兼容层](FFI_COMPAT_LAYER.md) - Dart* 函数兼容层说明
- [Tim2Tox Bootstrap 与轮询](BOOTSTRAP_AND_POLLING.md) - Bootstrap 节点、联网建立与 poll loop
- [Tim2Tox ToxAV 与 Signaling](TOXAV_AND_SIGNALING.md) - 通话与实例路由
