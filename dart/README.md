# Tim2Tox Dart Package

Tim2Tox Dart 包提供了 Tim2Tox framework 的 Dart 绑定和 SDK Platform 实现。

## 目录结构

```
tim2tox/dart/
├── lib/
│   ├── ffi/
│   │   └── tim2tox_ffi.dart          # FFI 绑定层
│   ├── models/
│   │   └── chat_message.dart         # 消息模型
│   ├── service/
│   │   └── ffi_chat_service.dart     # 服务层（需要重构）
│   ├── sdk/
│   │   └── tim2tox_sdk_platform.dart # SDK Platform 实现
│   ├── interfaces/
│   │   ├── preferences_service.dart  # 偏好设置接口
│   │   ├── logger_service.dart       # 日志接口
│   │   └── bootstrap_service.dart    # Bootstrap 接口
│   └── tim2tox_dart.dart             # 主导出文件
├── pubspec.yaml
└── README.md
```

## 使用方式

### 1. 在项目中添加依赖

```yaml
dependencies:
  tim2tox_dart:
    path: ../tim2tox/dart
```

### 2. 实现接口

```dart
import 'package:tim2tox_dart/interfaces/preferences_service.dart';
import 'package:shared_preferences/shared_preferences.dart';

class MyPreferencesService implements PreferencesService {
  final SharedPreferences _prefs;
  MyPreferencesService(this._prefs);
  
  @override
  Future<String?> getString(String key) => _prefs.getString(key);
  
  // ... 实现其他方法
}
```

### 3. 初始化服务

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

### 4. 设置 SDK Platform

```dart
import 'package:tim2tox_dart/sdk/tim2tox_sdk_platform.dart';

TencentCloudChatSdkPlatform.instance = Tim2ToxSdkPlatform(
  ffiService: ffiService,
);
```

## 状态

✅ **核心功能已完成**

- ✅ FFI 绑定层已完成
- ✅ 抽象接口已定义
- ✅ 服务层已重构（使用抽象接口）
- ✅ SDK Platform 已实现（移除对客户端特定代码的依赖）
- ✅ 消息ID统一格式（`timestamp_userID`）
- ✅ 失败消息处理机制（超时检测、离线检测、持久化存储）

## 核心特性

### 消息ID管理

所有消息ID使用 `timestamp_userID` 格式（毫秒级时间戳）：
- 格式：`<timestamp>_<userID>`
- 确保唯一性和一致性
- 使用 `ffiService.selfId` 生成用户ID部分

### 失败消息处理

- **超时机制**：文本消息5秒超时，文件消息根据大小动态计算
- **离线检测**：发送前检查联系人是否在线
- **持久化存储**：失败消息保存到本地存储
- **状态恢复**：客户端重启后自动恢复失败消息状态

