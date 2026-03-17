# Tim2Tox 与 V2TIM 的关系
> 语言 / Language: [中文](PLATFORM_VS_V2TIM_AND_CONVERSATION_LISTENER.md) | [English](PLATFORM_VS_V2TIM_AND_CONVERSATION_LISTENER.en.md)


## 1. 整体架构（二进制替换）

```
┌─────────────────────────────────────────────────────────────────────────────┐
│  App / UIKit / 测试                                                           │
│  - TencentCloudChatSdkPlatform.instance = Tim2ToxSdkPlatform(...)             │
│  - TIMConversationManager.instance.addConversationListener(...) 等            │
└───────────────────────────────────────────┬─────────────────────────────────┘
                                            │
┌───────────────────────────────────────────▼─────────────────────────────────┐
│  tencent_cloud_chat_sdk（未替换，仍为原包）                                    │
│  - TencentCloudChatSdkPlatform（接口）                                        │
│  - TIMConversationManager / TIMMessageManager 等 adapter                       │
│  - 所有 API 最终调用 platform.xxx() 或 NativeLibraryManager（FFI）             │
└───────────────────────────────────────────┬─────────────────────────────────┘
                    │                                    │
        platform 接口调用                     FFI / Native 调用
                    │                                    │
┌───────────────────▼──────────────────┐   ┌────────────▼─────────────────────┐
│  Tim2ToxSdkPlatform（Dart）           │   │  tim2tox_ffi / dart_compat（C++）  │
│  - 实现 TencentCloudChatSdkPlatform   │   │  - SafeGetV2TIMManager()          │
│  - addConversationListener 等        │   │  - manager->GetConversationManager│
│  - _conversationListeners 等          │   │  - conv_manager->AddConversation  │
│  - globalCallback 分发（instance_id） │   │  - SendCallbackToDart(globalCallback)│
└───────────────────┬──────────────────┘   └────────────┬─────────────────────┘
                    │                                    │
                    │         Dart_PostCObject_DL        │
                    └────────────────┬───────────────────┘
                                     │
┌────────────────────────────────────▼─────────────────────────────────────────┐
│  Native 层（C++）                                                              │
│  - V2TIMManagerImpl（每实例一个，多实例时 instance_id 区分）                    │
│  - V2TIMConversationManagerImpl::GetInstance()（单例，但 SetManagerImpl(this)）│
│  - V2TIMSignalingManagerImpl（每实例一个，GetSignalingManager() 每实例独立）    │
│  - ToxManager / toxcore                                                       │
└─────────────────────────────────────────────────────────────────────────────┘
```

- **二进制替换**：不替换 tencent_cloud_chat_sdk 的类，只替换 **平台实例** 和 **Native 实现**。  
- **Tim2ToxSdkPlatform**：实现 `TencentCloudChatSdkPlatform` 接口，所有“平台方法”（如 `addConversationListener`、`getConversationList`）由它实现；同时处理来自 Native 的 `globalCallback`，按 `instance_id` 分发给各 listener。  
- **V2TIM***：C++ 里的业务实现（会话、消息、群组、信令等）。Dart 通过 FFI 调 Native，Native 内部用 `V2TIMManagerImpl`、`GetConversationManager()` 等；回调通过 dart_compat 发 `globalCallback` 到 Dart，由 **Tim2ToxSdkPlatform** 按 instance 分发。

## 2. Tim2ToxSdkPlatform 与 V2TIM* 的对应关系

| 能力           | Dart 侧（Platform/Adapter）              | C++ 侧（V2TIM*） |
|----------------|------------------------------------------|------------------|
| 会话列表       | Platform.getConversationList / provider  | V2TIMConversationManagerImpl（GetConversationList、缓存、Pin） |
| 会话监听       | Platform.addConversationListener        | V2TIMConversationManagerImpl::AddConversationListener + dart_compat 注册 DartConversationListenerImpl |
| 会话变更回调   | Platform._conversationListeners + globalCallback 分发 | C++ 调 listeners → dart_compat → SendCallbackToDart(globalCallback) |
| 消息           | Platform + FFI send/poll                | V2TIMMessageManagerImpl + Tox |
| 群组           | Platform + FFI                          | V2TIMGroupManagerImpl |
| 信令           | Platform + FFI                          | V2TIMSignalingManagerImpl（每实例） |

- **会话监听**：  
  - **C++**：`V2TIMConversationManagerImpl` 是单例，但每次 `GetConversationManager()` 会 `SetManagerImpl(this)`，用当前实例做后续操作。dart_compat 用当前实例的 manager 拿到 `conv_manager`，再 `AddConversationListener(DartConversationListenerImpl)`；C++ 会话变更时通知这些 listener，再通过 globalCallback 发到 Dart。  
  - **Dart**：`Tim2ToxSdkPlatform.addConversationListener` 把 listener 存到 `_conversationListeners`（及按实例的列表）；当收到 globalCallback(ConversationEvent) 时，由 Platform 按 instance_id 分发给这些 listener。  
- 因此：**会话监听的“来源”是 C++ V2TIMConversationManagerImpl + dart_compat；“接收与分发”在 Dart Tim2ToxSdkPlatform。**

## 3. addConversationListener 实现位置评估

### 3.1 在 V2TIMConversationManagerImpl 里“实现”AddConversationListener？

- **现状**：C++ 已实现。`AddConversationListener` 只是把 `V2TIMConversationListener*` 放进 `listeners_`；dart_compat 在 Dart 侧每次 set 各种 Conv 回调时，会 `GetOrCreateConversationListener()` 得到一个 `DartConversationListenerImpl` 并 `conv_manager->AddConversationListener(listener)`。  
- **结论**：C++ 端不需要再“实现”AddConversationListener，已满足：C++ 会话变化 → 通知 listeners → dart_compat 发 globalCallback → Dart 分发。

### 3.2 在 Tim2ToxSdkPlatform 里实现 addConversationListener？

- **现状**：Platform 接口要求实现 `addConversationListener`；Tim2ToxSdkPlatform **已实现**：把 listener 放入 `_conversationListeners` 和 `_instanceConversationListeners[id]`。  
- **问题**：测试报错 “addConversationListener() has not been implemented” 的**根因**是**调用时机**：  
  - 在 `Tim2ToxSdkPlatform` 的**构造函数**里调用了 `_setupInternalConversationListener()`。  
  - 其中会执行 `TIMConversationManager.instance.addConversationListener(listener: internalListener)`。  
  - 而 adapter 里会再调 `TencentCloudChatSdkPlatform.instance.addConversationListener(...)`。  
  - 此时构造函数尚未返回，`TencentCloudChatSdkPlatform.instance` 还未被赋值为当前 `Tim2ToxSdkPlatform`，仍是默认平台（或旧实例），其默认实现直接抛 UnimplementedError。  
- **结论**：应在 **Tim2ToxSdkPlatform** 侧修复，而不是在 C++ 再实现一层。可选两种方式：  
  1. **不再在构造函数里把 internalListener 同步到 adapter**：只保留 `this.addConversationListener(listener: internalListener)`，不调用 `TIMConversationManager.instance.addConversationListener(internalListener)`（internal 仅需在 Platform 的 _conversationListeners 中，供 globalCallback 分发使用）。  
  2. **延后同步**：用 `Future.microtask(() => TIMConversationManager.instance.addConversationListener(listener: internalListener))`，在赋值 `instance = Tim2ToxSdkPlatform(...)` 之后再执行，这样 adapter 调到的就是当前 Platform 的 addConversationListener。

### 3.3 推荐

- **实现位置**：保持并沿用 **Tim2ToxSdkPlatform** 的 addConversationListener 实现；C++ 端 **V2TIMConversationManagerImpl::AddConversationListener** 保持现状即可。  
- **修复点**：在 Tim2ToxSdkPlatform 的 `_setupInternalConversationListener` 中，避免在“instance 尚未被赋值”时通过 adapter 调用 platform.addConversationListener。推荐做法：**仅调用 `this.addConversationListener(listener: internalListener)`，删除或延后对 `TIMConversationManager.instance.addConversationListener(internalListener)` 的调用**（若需与 adapter 列表一致，可用上述 microtask 延后）。

## 4. 小结

- **二进制替换下**：Tim2ToxSdkPlatform 是“平台实现”，V2TIM* 是“Native 能力实现”；两者通过 FFI + globalCallback 协作，Platform 负责按 instance_id 分发回 Dart listener。  
- **会话监听**：C++ 已有 AddConversationListener 与 dart_compat 注册；Dart 端由 Tim2ToxSdkPlatform 实现并持有 listener 列表，并在收到 globalCallback 时分发。  
- **addConversationListener 未实现报错**：来自“构造函数里通过 adapter 调 platform 时 instance 尚未指向当前 Platform”；应在 **Tim2ToxSdkPlatform** 内通过调整调用顺序或延后同步解决，而不是在 V2TIMConversationManagerImpl 再实现一层。
