# Tim2Tox 更新日志

## [未发布] - SDK 通用化重构 + Binary Replacement 测试覆盖

### 重大改进

#### SDK 通用化重构 (2026-02-10)

移除 SDK 中所有 tim2tox 专有代码，改为通用扩展点，降低 SDK 升级成本。

- ✅ **库名可配置**: `setNativeLibraryName()` 替代硬编码 `'tim2tox_ffi'`，默认值恢复为 `'dart_native_imsdk'`（原始 SDK 名）
- ✅ **通用 callback hook**: `NativeLibraryManager.customCallbackHandler` 静态字段，替代 SDK 内 3 个 tim2tox 专有 case handler（`clearHistoryMessage`/`groupQuitNotification`/`groupChatIdStored`）
- ✅ **统一平台标识**: `isTim2ToxPlatform` 全部替换为 `isCustomPlatform`，适用于任何自定义平台
- ✅ **清理调试日志**: 删除 SDK 中 38 条 `print('[NativeLibraryManager]...)` 和 model 层调试 print

**SDK 中不再包含任何 `isTim2ToxPlatform`、`tim2tox_ffi` 硬编码、或 tim2tox 专有 callback handler。**

#### Tim2Tox 侧适配

- ✅ **`Tim2ToxSdkPlatform`**: 构造函数注册 `customCallbackHandler`，接管 3 个自定义 callback
- ✅ **`toxee`**: `main()` 中调用 `setNativeLibraryName('tim2tox_ffi')`
- ✅ **`auto_tests`**: `setupTestEnvironment()` 自动配置库名

#### Binary Replacement 路径测试覆盖 (Phase 13)

新增 15 个测试用例，覆盖 `NativeLibraryManager` 静态 listener 分发路径（`instance_id == 0` 单实例场景）。

- ✅ **FFI Callback 注入**: C++ 层新增 `tim2tox_ffi_inject_callback` 函数，通过 `SendCallbackToDart` 注入 JSON callback 到 Dart ReceivePort
- ✅ **Native Callback Dispatch** (5 用例): NetworkStatus→onConnectSuccess/onConnecting、ReceiveNewMessage→onRecvNewMessage、ConversationEvent→onConversationChanged、FriendAddRequest→onFriendApplicationListAdded
- ✅ **Custom Callback Handler** (6 用例): 验证 `customCallbackHandler` 注册/触发/null 安全、clearHistoryMessage/groupQuitNotification/groupChatIdStored 路由
- ✅ **Library Loading** (4 用例): 验证 `setNativeLibraryName` 配置、registerPort 成功、callback 到达 Dart、injectCallback 返回值
- ✅ **`run_tests_ordered.sh` Phase 13**: `./run_tests_ordered.sh 13` 或 `./run_tests_ordered.sh BINARY`

### 涉及文件

| 文件 | 操作 | 说明 |
|------|------|------|
| `native_library_manager.dart` | 修改 | `setNativeLibraryName()` + `customCallbackHandler` + 删除 tim2tox 专有代码 |
| `tencent_cloud_chat_sdk_platform_interface.dart` | 修改 | 删除 `isTim2ToxPlatform` |
| `tim_manager.dart` / `tim_friendship_manager.dart` / `tim_group_manager.dart` / `tim_message_manager.dart` | 修改 | `isTim2ToxPlatform` → `isCustomPlatform` |
| `v2_tim_message.dart` / `v2_tim_text_elem.dart` | 修改 | 删除调试 print |
| `tim2tox_sdk_platform.dart` | 修改 | 注册 `customCallbackHandler` |
| `toxee/main.dart` | 修改 | 添加 `setNativeLibraryName('tim2tox_ffi')` |
| `test_fixtures.dart` | 修改 | 添加 `setupNativeLibraryForTim2Tox()` |
| `tim2tox_ffi.cpp` / `tim2tox_ffi.h` | 修改 | 新增 `tim2tox_ffi_inject_callback` |
| `tim2tox_ffi.dart` | 修改 | 新增 `injectCallback` Dart 绑定 |
| `scenarios_binary/` (3 个文件) | 新增 | 15 个 binary replacement 测试 |
| `run_tests_ordered.sh` | 修改 | 新增 Phase 13 |

---

## [早期] - 测试和稳定性改进

### 重大改进

#### 测试套件完善

- ✅ **所有 Auto Tests 通过**: 46个测试场景全部通过，包括基础功能、好友管理、消息、群组和其他功能
- ✅ **自动接受好友请求**: 实现了类似 `c-toxcore` 的 `tox_friend_add_norequest` 功能，测试节点自动接受好友请求
- ✅ **多实例支持完善**: 每个测试实例拥有独立的 listener，确保回调正确路由到对应实例

#### 回调系统修复

- ✅ **JSON 参数格式统一**: 修复了所有回调的 JSON 格式，确保 `json_param` 字段正确传递
- ✅ **List<T> 类型回调修复**: 修复了所有返回 `List<T>` 类型的回调，直接传递 JSON 数组而不是包装对象
- ✅ **字段名称对齐**: 确保 C++ 层 JSON 字段名称与 Dart 层 `fromJson` 期望的字段名称一致
- ✅ **在线状态修复**: 修复了客户端在线状态显示问题，确保使用完整的 76 字符 Tox ID

#### 多实例架构改进

- ✅ **独立 Listener 管理**: 每个 `tim2tox` 实例拥有独立的 listener 集合（SDK、Friendship、Group、Message 等）
- ✅ **实例 ID 映射**: 实现了从 `V2TIMManagerImpl*` 到 `instance_id` 的反向映射
- ✅ **回调路由优化**: 确保回调正确路由到对应的实例，避免所有实例收到相同的回调

### 修复

#### 回调相关修复

- ✅ 修复 `DartFriendInfoVectorCallback` 使用 `json_param` 而不是 `data`
- ✅ 修复 `DartGroupInfoVectorCallback` 直接传递 JSON 数组
- ✅ 修复 `DartUserStatusVectorCallback` 使用正确的字段名称
- ✅ 修复 `DartMessageVectorCallback` 直接传递 JSON 数组
- ✅ 修复 `DartConversationCallback` 返回 JSON 数组格式
- ✅ 修复 `DartGroupInfoResultVectorCallback` 生成符合 Dart 期望的 JSON 结构
- ✅ 修复 `UserStatusVectorToJson` 使用 Dart 期望的字段名称

#### 在线状态修复

- ✅ 修复 `HandleSelfConnectionStatus` 通知在线状态时使用完整的 76 字符 Tox ID
- ✅ 修复 `Login` 函数在已连接时通知在线状态
- ✅ 修复 SDK Listener 注册，确保 `onUserStatusChanged` 回调被正确调用
- ✅ 修复 JSON 字段名称匹配问题（`user_status_identifier`, `user_status_status_type` 等）

#### 好友管理修复

- ✅ 修复 `TOX_ERR_FRIEND_ADD_SET_NEW_NOSPAM` (错误码 4) 处理，将其视为成功
- ✅ 修复 `HandleFriendRequest` 使用实例特定的 listener
- ✅ 修复 `AcceptFriendApplication` 处理已发送的好友请求

#### 测试相关修复

- ✅ 修复测试实例清理，确保正确恢复默认实例
- ✅ 修复 `testInstanceHandle` 在超时后变为 null 的问题
- ✅ 修复好友列表查询返回空列表的问题

### 代码清理

- ✅ **调试日志清理**: 清理了所有不必要的 `fprintf(stdout, ...)` 调试输出
- ✅ **保留错误日志**: 保留了所有 `fprintf(stderr, ...)` 错误日志和 `V2TIM_LOG` 日志
- ✅ **代码优化**: 移除了临时调试代码和冗余日志

### 文档更新

- ✅ 更新测试状态文档
- ✅ 更新 README 中的测试状态
- ✅ 更新架构文档中的多实例支持说明

### 统计

- **测试通过率**: 100% (46/46 测试场景)
- **回调修复**: 15+ 个回调函数
- **代码清理**: 清理了 50+ 处调试日志

## [未发布] - 模块化重构

### 重大变更

#### FFI 层模块化拆分

- ✅ **完成模块化拆分**: 将 `dart_compat_layer.cpp` (原3200+行) 拆分为13个功能模块
- ✅ **代码组织优化**: 每个模块专注于特定功能，提高可维护性
- ✅ **主文件清理**: `dart_compat_layer.cpp` 现在仅29行，仅包含头文件和说明

#### 新增模块文件

1. **基础架构**:
   - `dart_compat_internal.h` - 共享声明和前置声明
   - `dart_compat_utils.cpp` - 工具函数和全局变量

2. **监听器和回调**:
   - `dart_compat_listeners.cpp` - 监听器实现和回调注册
   - `dart_compat_callbacks.cpp` - 回调类实现

3. **功能模块**:
   - `dart_compat_sdk.cpp` - SDK初始化和认证
   - `dart_compat_message.cpp` - 消息相关功能
   - `dart_compat_friendship.cpp` - 好友相关功能
   - `dart_compat_conversation.cpp` - 会话相关功能
   - `dart_compat_group.cpp` - 群组相关功能
   - `dart_compat_user.cpp` - 用户相关功能
   - `dart_compat_signaling.cpp` - 信令相关功能
   - `dart_compat_community.cpp` - 社区相关功能（待完善）
   - `dart_compat_other.cpp` - 其他杂项功能（待完善）

### 改进

- ✅ **编译效率**: 修改单个模块只需重新编译该模块
- ✅ **代码可读性**: 每个模块文件大小合理（最大约1150行）
- ✅ **团队协作**: 不同开发者可以并行开发不同模块
- ✅ **构建验证**: 所有模块编译通过，无链接错误

### 函数实现

- ✅ **核心功能**: 已实现约131个核心函数（~58%）
- ✅ **实际使用**: 所有实际使用的函数（约68个）已全部实现
- ⏳ **待完善**: 约142个函数待实现（主要是信令、社区和其他杂项功能）

### 文档更新

- ✅ 新增 [模块化文档](doc/MODULARIZATION.md)
- ✅ 更新 [FFI 兼容层文档](doc/FFI_COMPAT_LAYER.md)
- ✅ 更新 [架构文档](doc/ARCHITECTURE.md)
- ✅ 更新 [开发指南](doc/DEVELOPMENT_GUIDE.md)
- ✅ 更新 [模块化状态报告](ffi/MODULARIZATION_STATUS.md)

### 修复

- ✅ 修复 `dart_compat_callbacks.cpp` 中的注释错误
- ✅ 修复 `dart_compat_signaling.cpp` 中的类型错误（移除不存在的字段和类型）
- ✅ 修复 `tim2tox_ffi.cpp` 中的重复符号问题（重命名 `g_signaling_listener`）

### 统计

- **模块文件数**: 13个 C++ 文件
- **总代码量**: 约6764行（分布在13个模块中）
- **主文件大小**: 29行（仅包含头文件和说明）
- **最大模块**: `dart_compat_listeners.cpp` (约1150行)
- **最小模块**: `dart_compat_community.cpp` 和 `dart_compat_other.cpp` (各约15行)

