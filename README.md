# Tim2Tox

Tim2Tox 是一个连接 Tencent Cloud Chat UIKit 与 Tox 的可复用兼容层。它基于 c-toxcore 实现，底层对接 Tox 网络协议，上层提供与 V2TIM 风格兼容的 C++ API、C FFI 和 Dart 封装。

## 文档 / Documentation

- 中文文档索引：[doc/README.md](doc/README.md)
- English documentation index: [doc/README.en.md](doc/README.en.md)
- 架构说明：[doc/ARCHITECTURE.md](doc/ARCHITECTURE.md) / [doc/ARCHITECTURE.en.md](doc/ARCHITECTURE.en.md)
- API 参考：[doc/API_REFERENCE.md](doc/API_REFERENCE.md) / [doc/API_REFERENCE.en.md](doc/API_REFERENCE.en.md)
- Bootstrap 与轮询机制：[doc/BOOTSTRAP_AND_POLLING.md](doc/BOOTSTRAP_AND_POLLING.md) / [doc/BOOTSTRAP_AND_POLLING.en.md](doc/BOOTSTRAP_AND_POLLING.en.md)
- 相关 toxee 文档：[../toxee/doc/README.md](../toxee/doc/README.md) / [../toxee/doc/README.en.md](../toxee/doc/README.en.md)

## 架构

Tim2Tox 可以被不同的 Flutter 聊天客户端复用：

```
UIKit (Flutter) → Tim2Tox Dart Package → Tim2Tox (C++) → Tox (c-toxcore)
```

### 目录结构

```
tim2tox/
├── source/                 # C++ 核心实现（V2TIM API）
│   ├── V2TIMManagerImpl.*  # V2TIM Manager 实现
│   ├── V2TIMMessageManagerImpl.*  # 消息管理
│   ├── V2TIMFriendshipManagerImpl.*  # 好友管理
│   ├── V2TIMGroupManagerImpl.*  # 群组管理
│   └── ToxManager.*        # Tox 核心管理
├── include/                # 公共头文件（V2TIM API 定义）
├── ffi/                    # C/C++ FFI 接口层
│   ├── tim2tox_ffi.h       # C 接口定义
│   ├── tim2tox_ffi.cpp     # C++ 实现
│   └── CMakeLists.txt      # FFI 库构建配置
├── dart/                   # Dart 包（Flutter 绑定）
│   ├── lib/
│   │   ├── ffi/            # Dart FFI 绑定层
│   │   ├── service/        # 服务层
│   │   ├── sdk/            # SDK Platform 实现
│   │   ├── models/         # 数据模型
│   │   └── interfaces/     # 抽象接口
│   └── pubspec.yaml
├── third_party/            # c-toxcore 依赖
└── example/               # C++ 示例程序
```

## 集成方案

Tim2Tox 同时支持二进制替换路径和 Platform/FfiChatService 路径；当前 toxee 采用的是**混合架构**。

### 当前客户端实现：混合架构

**关键点**：
- 二进制替换路径继续承接 `TIMManager.instance` → `NativeLibraryManager` → Dart* 函数的兼容调用链。
- Platform 路径通过 `Tim2ToxSdkPlatform` 与 `FfiChatService` 补足历史消息查询、特殊回调、会话联动、Bootstrap/polling、通话桥等能力。
- Tim2Tox 本身仍保持“二进制替换能力 + Platform 能力”并存，客户端按场景选择调用面。

**路径概览**：
```
UIKit SDK
  ├─ NativeLibraryManager → dart_compat_layer.cpp → V2TIM*Manager / ToxManager
  └─ Tim2ToxSdkPlatform → FfiChatService / Tim2ToxFfi → V2TIM*Manager / ToxManager
```

**详细说明**：
- Tim2Tox 架构：[doc/ARCHITECTURE.md](doc/ARCHITECTURE.md)
- 二进制替换方案：[doc/BINARY_REPLACEMENT.md](doc/BINARY_REPLACEMENT.md)
- FFI 兼容层：[doc/FFI_COMPAT_LAYER.md](doc/FFI_COMPAT_LAYER.md)

## 特性

- **多语言支持**：
  - C++ 核心实现（静态库 `libtim2tox.a`）
  - C FFI 接口（动态库 `libtim2tox_ffi.dylib`）
  - Dart 包（`tim2tox_dart`）提供 Flutter 绑定

- **架构设计**：
  - 清晰的层次分离：C++ 核心 → FFI 接口 → Dart 绑定
  - **抽象接口设计**：完全独立的 framework，通过接口注入客户端依赖
  - **依赖注入**：支持 Preferences、Logger、Bootstrap、EventBus、ConversationManager 等接口
  - **两种集成方案**：支持二进制替换和 Platform 接口两种集成方式
  - **可复用性**：不绑定任何客户端特定代码，可被任何 Flutter 客户端使用

- **功能特性**：
  - 不依赖音视频功能
  - 支持 IPv6
  - 完整的错误处理和日志系统
  - 支持 IRC 通道桥接（可选）
  - 完整的 V2TIM API 实现
  - 消息历史管理
  - 好友和群组管理
  - 文件传输支持
  - 消息ID统一格式：使用 `timestamp_userID` 格式（毫秒级时间戳）
  - 离线消息检测：自动检测离线联系人并立即标记消息为失败
  - 消息超时机制：文本消息5秒超时，文件消息根据大小动态计算超时时间
  - **多实例支持**：支持创建多个独立的 Tox 实例，每个实例拥有独立的网络端口、DHT ID 和持久化路径（主要用于测试场景）

## 依赖

### C++ 构建依赖
- CMake >= 3.4.1
- C++20 兼容的编译器
- [c-toxcore](https://github.com/TokTok/c-toxcore)
- libsodium

### Dart 包依赖
- Dart SDK >= 3.0.0
- Flutter SDK
- `ffi` 包
- `tencent_cloud_chat_sdk` 包

## 构建

### 构建 C++ Framework

#### 使用脚本构建

```bash
./build.sh
```

#### 手动构建

```bash
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_FFI=ON
make -j$(nproc)
```

构建产物：
- `build/source/libtim2tox.a` - 静态库
- `build/ffi/libtim2tox_ffi.dylib` - FFI 动态库（macOS）

#### CMake 选项

主要的构建选项：

- `BUILD_FFI`: 是否构建 FFI 动态库（默认：ON）
- `BUILD_TOXAV`: 是否构建音视频支持（默认：OFF）
- `USE_IPV6`: 启用 IPv6 支持（默认：ON）
- `ENABLE_STATIC`: 构建静态库（默认：ON）
- `ENABLE_SHARED`: 构建动态库（默认：OFF）

日志级别选项：

- `ERROR`: 启用错误日志（默认：ON）
- `WARNING`: 启用警告日志（默认：ON）
- `INFO`: 启用信息日志（默认：ON）
- `DEBUG`: 启用调试日志（默认：OFF）
- `TRACE`: 启用跟踪日志（默认：OFF）

### 构建 Dart 包

```bash
cd dart
flutter pub get
```

## 使用

### C++ 使用

#### 包含头文件

```cpp
#include <V2TIMManager.h>
```

#### 基本用法

```cpp
#include <V2TIMManager.h>

int main() {
    // 初始化 SDK
    V2TIMManager::GetInstance()->InitSDK(...);
    
    // 登录
    V2TIMManager::GetInstance()->Login(...);
    
    // 使用其他 Manager
    auto msgMgr = V2TIMManager::GetInstance()->GetMessageManager();
    // ...
    
    return 0;
}
```

### Flutter/Dart 使用

#### 多实例支持（测试场景）

Tim2Tox 支持创建多个独立的 Tox 实例，每个实例拥有独立的网络端口、DHT ID 和持久化路径。这对于测试场景特别有用，允许在同一进程中运行多个节点进行互操作测试。

**测试实例管理 API**（FFI 层）：

推荐使用 **实例作用域** `Tim2ToxInstance`，用 `runWithInstance` / `runWithInstanceAsync` 包住多实例调用，避免到处手写 `setCurrentInstance`，并保证“当前实例”在调用期间不被打断：

```dart
import 'package:tim2tox_dart/instance/tim2tox_instance.dart';

// 创建测试实例后得到 instanceHandle
final instance = Tim2ToxInstance(instanceHandle);

// 推荐：用 runWithInstanceAsync 包住该实例上的 SDK 调用
await instance.runWithInstanceAsync(() async {
  await TIMManager.instance.initSDK(...);
  await TIMManager.instance.login(...);
});
// 同步调用用 runWithInstance
instance.runWithInstance(() => TIMManager.instance.someSyncCall());
```

若需直接使用 FFI，可手动 `setCurrentInstance(handle)` 后再调 SDK，但多实例场景下更易出错，建议用上述实例作用域方式。

```dart
import 'package:tim2tox_dart/ffi/tim2tox_ffi.dart' as ffi_lib;

final ffiInstance = ffi_lib.Tim2ToxFfi.open();
final initPathPtr = '/path/to/instance/data'.toNativeUtf8();
final instanceHandle = ffiInstance.createTestInstanceNative(initPathPtr);

ffiInstance.setCurrentInstance(instanceHandle);
await TIMManager.instance.initSDK(...);
await TIMManager.instance.login(...);

ffiInstance.destroyTestInstance(instanceHandle);
```

**注意**：
- 生产环境应用通常使用默认实例（通过 `V2TIMManagerImpl::GetInstance()`），无需创建测试实例
- 测试实例主要用于自动化测试场景，如 `tim2tox/auto_tests` 中的多节点测试
- 每个测试实例都有独立的持久化路径，确保数据隔离

**默认实例（生产环境）**：

对于生产环境应用（如 `toxee`），直接使用 `TIMManager.instance` 即可，它会自动使用默认实例：

```dart
// 生产环境：直接使用默认实例
await TIMManager.instance.initSDK(...);
await TIMManager.instance.login(...);
```

#### 1. 添加依赖

在 `pubspec.yaml` 中：

```yaml
dependencies:
  tim2tox_dart:
    path: ../tim2tox/dart
```

#### 2. 实现接口适配器

Framework 需要以下接口适配器：

**必需接口**：
- `ExtendedPreferencesService`: 偏好设置服务
- `LoggerService`: 日志服务
- `BootstrapService`: Bootstrap 节点服务

**可选接口**（用于高级功能）：
- `EventBusProvider`: 事件总线提供者
- `ConversationManagerProvider`: 会话管理器提供者

示例实现：

```dart
import 'package:tim2tox_dart/interfaces/extended_preferences_service.dart';
import 'package:shared_preferences/shared_preferences.dart';

class MyPreferencesService implements ExtendedPreferencesService {
  final SharedPreferences _prefs;
  MyPreferencesService(this._prefs);
  
  @override
  Future<String?> getString(String key) async => _prefs.getString(key);
  
  // ... 实现所有必需方法
}
```

#### 3. 初始化服务

```dart
import 'package:tim2tox_dart/tim2tox_dart.dart';

final prefsService = MyPreferencesService(await SharedPreferences.getInstance());
final loggerService = MyLoggerService();
final bootstrapService = MyBootstrapService();

final ffiService = FfiChatService(
  preferencesService: prefsService,
  loggerService: loggerService,
  bootstrapService: bootstrapService,
);

await ffiService.init();
```

#### 4. 设置 SDK Platform

```dart
import 'package:tim2tox_dart/sdk/tim2tox_sdk_platform.dart';

// 基本用法（仅必需接口）
TencentCloudChatSdkPlatform.instance = Tim2ToxSdkPlatform(
  ffiService: ffiService,
);

// 高级用法（包含可选接口）
final eventBusProvider = MyEventBusProvider();
final conversationManagerProvider = MyConversationManagerProvider();

TencentCloudChatSdkPlatform.instance = Tim2ToxSdkPlatform(
  ffiService: ffiService,
  eventBusProvider: eventBusProvider,
  conversationManagerProvider: conversationManagerProvider,
);
```

#### 5. 使用 UIKit

现在可以正常使用 Tencent Cloud Chat UIKit 组件，所有调用都会路由到 tim2tox。

**多实例支持**：参见 [多实例支持文档](doc/MULTI_INSTANCE_SUPPORT.md) 了解如何在测试场景中使用多实例功能。

## 示例项目

参考 `../toxee/` 目录，这是一个完整的 Flutter 客户端示例，展示了如何使用 tim2tox framework。

该示例包含：
- 完整的接口适配器实现（`lib/adapters/`）
- UIKit 集成示例
- 客户端特定的适配层（`lib/sdk_fake/`）
- 详细的架构文档（`doc/ARCHITECTURE.md`）

## 模块说明

### C++ 核心层 (`source/`)
- 实现了完整的 V2TIM API
- 管理 Tox 实例和生命周期
- 处理消息、好友、群组等核心功能

### FFI 接口层 (`ffi/`)
- 提供 C 接口，供 Dart FFI 调用
- 生成动态库 `libtim2tox_ffi.dylib`
- 不直接调用 Dart，使用事件队列机制
- **Dart* 函数兼容层** (模块化设计): 实现二进制替换方案，导出所有 Dart* 函数，完全兼容原生 IM SDK 的 FFI 接口
  - **模块化结构**: 已拆分为13个功能模块，提高可维护性
  - **核心模块**:
    - `dart_compat_internal.h`: 共享声明和前置声明
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

### Dart 包 (`dart/`)
- **FFI 绑定** (`lib/ffi/`): 直接调用 C FFI 库
- **服务层** (`lib/service/`): 高级服务，管理消息历史、轮询等
- **SDK Platform** (`lib/sdk/`): 实现 `TencentCloudChatSdkPlatform` 接口
- **抽象接口** (`lib/interfaces/`): 定义可注入的依赖接口
  - `PreferencesService` / `ExtendedPreferencesService`: 偏好设置接口
  - `LoggerService`: 日志接口
  - `BootstrapService`: Bootstrap 节点接口
  - `EventBus` / `EventBusProvider`: 事件总线接口
  - `ConversationManagerProvider`: 会话管理器接口
- **数据模型** (`lib/models/`): 共享的数据模型（ChatMessage、Fake* 模型等）

## FFI 兼容层（二进制替换方案）

Tim2Tox 实现了完整的 Dart* 函数兼容层，支持二进制替换方案，使得 Dart 层的 `NativeLibraryManager` 可以无缝切换到 tim2tox 后端，而无需修改任何 Dart 代码。

### 核心特性

- ✅ **纯二进制替换能力**: 在只走 `NativeLibraryManager` / Dart* 兼容层的场景下，Dart 业务代码可以保持不改
- ✅ **函数签名完全匹配**: 所有 Dart* 函数签名与原生 SDK 完全一致
- ✅ **按需实现**: 只实现实际使用的函数（约 68 个），而不是所有定义的函数
- ✅ **回调桥接**: 通过 JSON 消息和 `Dart_PostCObject_DL` 将 C++ 事件发送到 Dart 层

### 实现状态

- ✅ **基础设施**: 回调桥接、JSON 解析工具（100% 完成）
- ✅ **回调设置函数**: 66 个函数（100% 完成）
- ✅ **业务 API 函数**: ~131+ 个核心函数（~98% 完成）
- ✅ **Listener 实现**: 所有Listener基础实现完成（ConversationListener、GroupListener、SignalingListener、CommunityListener、AdvancedMsgListener、FriendshipListener、SDKListener）
- ✅ **模块化拆分**: 已完成，代码已拆分为13个功能模块，提高可维护性
- ✅ **构建验证**: C++兼容层编译通过，Flutter应用可以正常构建和运行
- ⏳ **函数完善**: 部分不常用函数待实现（约142个函数，主要是信令、社区和其他杂项功能）

详细实现状态请参考 [FFI 兼容层文档](doc/FFI_COMPAT_LAYER.md)。

## 文档

### 主要文档

- [API 参考](doc/API_REFERENCE.md) - V2TIM API 完整参考、FFI 接口文档、Dart 包 API 文档
- [开发指南](doc/DEVELOPMENT_GUIDE.md) - 如何添加新功能、代码结构、构建系统、测试指南
- [架构文档](doc/ARCHITECTURE.md) - 整体架构设计、模块划分、数据流图、关键设计决策
- [FFI 兼容层文档](doc/FFI_COMPAT_LAYER.md) - Dart* 函数兼容层说明、回调机制、JSON 消息格式、实现状态
- [模块化文档](doc/MODULARIZATION.md) - FFI 层模块化结构、模块说明、开发指南

### 实现状态

- [模块化文档](doc/MODULARIZATION.md) - FFI 模块拆分结构、模块职责与完成度

## 测试状态

### Auto Tests

Tim2Tox 包含完整的自动化测试套件，位于 `auto_tests/` 目录。

**当前状态**: ✅ **所有测试通过** (46/46 测试场景)

**测试分类**:
- **基础功能** (5个测试): SDK 初始化、登录、自我查询、保存/加载、多实例
- **好友功能** (7个测试): 好友请求、好友连接、好友查询、好友关系、好友删除、已读回执、防垃圾
- **消息功能** (4个测试): 消息发送/接收、消息查询、消息溢出、输入状态
- **群组功能** (9个测试): 群组创建/管理、群组消息、群组邀请、群组状态、群组同步、群组保存、群组话题、群组审核
- **其他功能** (21个测试): 设置名称、设置状态消息、会话管理、重连、保存好友、防垃圾、用户状态、事件、头像、文件传输、信令、会议、音视频等

**运行测试**:
```bash
cd auto_tests
./run_all_tests.sh
```

**测试特性**:
- ✅ 自动接受好友请求（类似 `c-toxcore` 的 `tox_friend_add_norequest`）
- ✅ 多实例支持（每个测试节点使用独立的 Tox 实例）
- ✅ 本地 Bootstrap（第一个节点作为其他节点的 Bootstrap）
- ✅ 完整的场景覆盖（从基础功能到复杂交互）

详细测试文档请参考 [auto_tests/README.md](auto_tests/README.md)。

## 开发指南

### 添加新功能

1. **C++ 层**：在 `source/` 中实现 V2TIM API
2. **FFI 层**：在 `ffi/tim2tox_ffi.h` 和 `ffi/tim2tox_ffi.cpp` 中添加 C 接口
3. **Dart 层**：在 `dart/lib/ffi/tim2tox_ffi.dart` 中添加 FFI 绑定，在服务层添加高级 API

### 测试

```bash
# 运行 C++ 测试
cd build
make test
ctest

# 运行 Dart 测试
cd dart
flutter test
```

## 故障排除

### 构建问题

#### 找不到 libsodium

**macOS**:
```bash
brew install libsodium
```

**Linux**:
```bash
sudo apt-get install libsodium-dev
# 或
sudo yum install libsodium-devel
```

**Windows**:
```bash
vcpkg install libsodium
```

#### CMake 配置失败

- 检查 CMake 版本: `cmake --version` (需要 >= 3.4.1)
- 检查编译器版本: 需要支持 C++20
- 检查依赖库路径是否正确

#### 链接错误

- 检查所有依赖库是否已正确安装
- 检查 CMakeLists.txt 中的链接配置
- 检查库的搜索路径

### 运行时问题

#### 动态库加载失败

**macOS**:
- 检查动态库路径是否正确
- 使用 `otool -L` 检查依赖
- 确保使用 `install_name_tool` 修复路径

**Linux**:
- 检查 `LD_LIBRARY_PATH` 环境变量
- 使用 `ldd` 检查依赖

**Windows**:
- 检查 DLL 路径
- 确保所有依赖 DLL 在同一目录

#### FFI 函数符号未找到

- 检查函数是否使用 `extern "C"` 声明
- 检查 CMakeLists.txt 中的导出配置
- 使用 `nm` 或 `objdump` 检查符号导出

#### 回调不触发

- 确保 `DartInitDartApiDL` 和 `DartRegisterSendPort` 已调用
- 检查 `IsDartPortRegistered()` 返回值
- 查看日志输出确认回调是否被调用

### 调试技巧

#### 启用调试日志

在 CMake 配置时启用：

```bash
cmake .. -DDEBUG=ON -DTRACE=ON
```

#### 检查函数符号

```bash
# macOS/Linux
nm -D libtim2tox_ffi.dylib | grep Dart

# Windows
dumpbin /EXPORTS tim2tox_ffi.dll
```

#### 查看回调消息

在 Dart 层添加日志：

```dart
receivePort.listen((message) {
  print('Received callback: $message');
  _handleNativeMessage(jsonDecode(message as String));
});
```

## 许可证

本项目采用 GPL-3.0 许可证。详见 [LICENSE](LICENSE) 文件。
