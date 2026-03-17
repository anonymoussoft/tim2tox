# Tim2Tox Bootstrap 与轮询
> 语言 / Language: [中文](BOOTSTRAP_AND_POLLING.md) | [English](BOOTSTRAP_AND_POLLING.en.md)


本文档说明 Tim2Tox 当前的 Bootstrap 节点加载、连接建立和轮询机制，并补充 toxee 在启动、设置页和局域网 Bootstrap 场景下的接入方式。

## 1. 范围

这条链路横跨两层：

- `toxee`：负责选择和保存当前 Bootstrap 节点、决定启动顺序、提供局域网 Bootstrap UI。
- `tim2tox/dart`：负责把保存下来的 `host/port/publicKey` 应用到 Tox 实例，并通过 `startPolling()` 驱动连接状态、消息、文件与 AV 事件。

需要注意的是，Tim2Tox 内部并没有单独的“局域网 Bootstrap 模式”分支。框架真正消费的始终只是一个当前 Bootstrap 节点三元组。

## 2. Bootstrap 节点来源

当前存在三种来源：

1. 手动输入
2. 自动从 `https://nodes.tox.chat/json` 拉取
3. 局域网内的本地 Bootstrap 服务

toxee 侧相关代码：

- `lib/util/bootstrap_nodes.dart`：从公共节点列表拉取节点，失败时退回硬编码 fallback 列表。
- `lib/ui/settings/bootstrap_settings_section.dart`：管理 `auto` / `manual` / `lan` 三种模式。
- `lib/ui/settings/bootstrap_nodes_page.dart`：列出在线节点、做探测、切换当前节点。
- `lib/util/lan_bootstrap_service.dart`：启动或探测本地 Bootstrap 服务。

Tim2Tox 侧相关代码：

- `dart/lib/interfaces/bootstrap_service.dart`：只暴露当前节点的 `host/port/publicKey` 读取与写入。
- `dart/lib/service/ffi_chat_service.dart`：在 `init()` 期间调用 `_loadAndApplySavedBootstrapNode()`。

## 3. 节点选择与保存

### 自动模式

toxee 在 `_StartupGate._decide()` 中会先检查 Bootstrap 模式：

- 非桌面平台如果检测到 `lan`，会强制回退到 `auto`
- `auto` 模式下，如果当前还没有保存节点，会先拉取公共节点列表，并把第一个在线节点写入 `Prefs.setCurrentBootstrapNode(...)`

这样做的目的是保证第一次启动时，在 `FfiChatService.init()` 之前就已经有一份可用节点配置。

### 手动模式

设置页手动输入节点后，会直接调用 `Prefs.setCurrentBootstrapNode(host, port, pubkey)`。之后 `FfiChatService.init()` 会在 `_loadAndApplySavedBootstrapNode()` 中读取并应用它。

### 局域网 Bootstrap 模式

局域网 Bootstrap 模式是 toxee 的配置层概念，不是 Tim2Tox 内部协议分支。当前实现里：

- 桌面端可以通过 `LanBootstrapServiceManager.startLocalBootstrapService(port)` 启动一个本地 Tox 实例作为 Bootstrap 服务
- 该服务会生成独立 profile、登录为 `BootstrapService`、取出 `udpPort` 和 `dhtId`，再启动自己的 `startPolling()`
- UI 层会显示它的 `ip/port/pubkey`

但 Tim2Tox 内部最终仍然只识别“当前 Bootstrap 节点”。因此局域网 Bootstrap 服务要真正参与连接，仍然需要把对应的 `host/port/pubkey` 写回当前节点配置。

## 4. Tim2Tox 如何应用 Bootstrap 节点

`FfiChatService.init()` 末尾会调用 `_loadAndApplySavedBootstrapNode()`：

- `manual` 模式：读取 `getCurrentBootstrapNode()` 并调用 `addBootstrapNode(...)`
- 其他模式：优先走 `BootstrapService.getBootstrapHost/Port/PublicKey()`；拿不到时再 fallback 到 `getCurrentBootstrapNode()`

`addBootstrapNode(host, port, publicKeyHex)` 会：

1. 调用 FFI `tim2tox_ffi_add_bootstrap_node`
2. 成功时把该节点保存回当前配置

这意味着在有 `service` 实例的页面里，“测试节点”不是纯只读操作。当前实现中，测试成功也会把该节点写成当前节点，因为它复用了 `addBootstrapNode(...)`。

## 5. 登录与联网状态

`FfiChatService.login(...)` 只负责：

- 调用原生 `login`
- 读取当前 `selfId`
- 立即用 `getSelfConnectionStatus()` 推送一次当前连接状态

它不保证此时已经完成联网。真正的连接变化依赖轮询队列中的 `conn:success` / `conn:failed` 事件。

toxee 启动时的顺序是：

1. `_StartupGate` 保证当前有可用 Bootstrap 节点
2. 初始化 `FfiChatService`
3. `FakeUIKit.startWithFfi(service)`
4. `_initTIMManagerSDK()`
5. `service.startPolling()`
6. 监听 `connectionStatusStream`
7. 在 20 秒内等到连接成功则预加载好友；否则超时后也进入首页

因此 `startPolling()` 是联网和消息消费的真正起点，不只是“后台刷新”。

## 6. 轮询循环

`FfiChatService.startPolling()` 当前会做这些事情：

1. 取消旧 poller 和旧 profile 保存定时器
2. 启动每 60 秒一次的 `saveToxProfileNow()`
3. 先推送一次当前 `_isConnected`
4. 按自适应间隔安排下一轮 poll

当前轮询间隔策略：

- 有文件传输：`50ms`
- 有多实例或共享实例轮询：`50ms`
- 最近 2 秒内有活动：`200ms`
- 空闲态：`1000ms`

每轮 poll 会：

1. 先尝试执行 `avIterate(instanceId)`
2. 按实例优先级轮询文本事件队列
3. 单次最多批量处理 200 个事件，避免文件传输时队列积压

多实例场景下会优先轮询非零实例，且倾向让接收方实例先被消费，以便 `file_request` 更快被处理。

## 7. 轮询队列里有哪些关键事件

当前文档最需要维护者关注的事件有：

- `conn:success` / `conn:failed`
- `c2c:` / `gtext:`
- `file_request:`
- `file_done:`
- `typing:`

其中：

- `conn:success` 会把 `_isConnected` 置为 `true`，并触发头像同步
- `file_request:` 是文件接收链路的关键前置事件；如果它没有及时被 poll 到，接收端就不能及时 `acceptFileTransfer`
- `file_done:` 会把临时接收状态收口到历史消息与 UI

这也是为什么文件传输期间会把 poll 间隔压到 `50ms`，并允许单轮批量 drain 队列。

## 8. toxee 侧的维护注意点

### 切换节点

`BootstrapNodesPage` 中切换节点的实际动作是：

1. `service.addBootstrapNode(...)`
2. `service.login(...)`

也就是“写入新节点并重新 login”，不是重建整个 `FfiChatService`。

### 测试节点

在设置页或节点列表页，如果 `service != null`，测试动作直接复用 `addBootstrapNode(...)`。这会带来一个实现层面的副作用：

- 测试成功时，当前节点配置也会被更新

如果只是登录页场景，没有现成 `service`，才会退化为普通 TCP 探测。

### 局域网 Bootstrap 服务

本地服务只在桌面端支持。当前实现主要是：

- 启动一个独立 Tox 实例
- 暴露它的 `ip/port/pubkey`
- 允许 UI 做存活探测

它本身不替代 `currentBootstrapNode` 这套配置模型。

## 9. 建议的维护顺序

如果你在改 Bootstrap、联网或轮询问题，建议按这个顺序读代码：

1. `toxee/lib/main.dart`
2. `toxee/lib/ui/settings/bootstrap_settings_section.dart`
3. `toxee/lib/util/bootstrap_nodes.dart`
4. `toxee/lib/util/lan_bootstrap_service.dart`
5. `tim2tox/dart/lib/interfaces/bootstrap_service.dart`
6. `tim2tox/dart/lib/service/ffi_chat_service.dart`

## 10. 相关文档

- [ARCHITECTURE.md](ARCHITECTURE.md)
- [API 参考](../api/API_REFERENCE.md)
- [../../toxee/doc/ACCOUNT_AND_SESSION.md](../../toxee/doc/ACCOUNT_AND_SESSION.md)
- [../../toxee/doc/IMPLEMENTATION_DETAILS.md](../../toxee/doc/IMPLEMENTATION_DETAILS.md)
