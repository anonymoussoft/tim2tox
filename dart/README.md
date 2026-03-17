# Tim2Tox Dart Package

Tim2Tox Dart 包提供了 Tim2Tox framework 的 Dart 绑定和 SDK Platform 实现。

> **完整集成入口**请从仓库根目录开始：
>
> - 项目定位、两种接入路径、集成 5 步摘要：[`../README.md`](../README.md)
> - 深度文档与推荐阅读路径：[`../doc/README.md`](../doc/README.md)
> - 构建说明（唯一入口）：[`../README_BUILD.md`](../README_BUILD.md)

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

### 1. 在项目中添加依赖（path 示例）

```yaml
dependencies:
  tim2tox_dart:
    path: ../tim2tox/dart
```

### 2. 实现注入接口（示意）

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

### 3. 初始化 FfiChatService（示意）

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

### 4. 设置 SDK Platform（如使用 Platform 路径）

```dart
import 'package:tim2tox_dart/sdk/tim2tox_sdk_platform.dart';

TencentCloudChatSdkPlatform.instance = Tim2ToxSdkPlatform(
  ffiService: ffiService,
);
```

## 说明

- 本 README 仅覆盖 Dart 包的目录结构与最小用法骨架；**接入决策与完整步骤**请以根 README 与 doc 索引为准，避免多处重复维护。

