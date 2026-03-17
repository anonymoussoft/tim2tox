# Tim2Tox Platform vs V2TIM and Conversation Listener
> Language: [Chinese](PLATFORM_VS_V2TIM_AND_CONVERSATION_LISTENER.md) | [English](PLATFORM_VS_V2TIM_AND_CONVERSATION_LISTENER.en.md)


## 1. Overall architecture (binary replacement)

```
┌─────────────────────────────────────────────────────────────────────────────┐
│App / UIKit / Testing │
│  - TencentCloudChatSdkPlatform.instance = Tim2ToxSdkPlatform(...)             │
│ - TIMConversationManager.instance.addConversationListener(...) etc. │
└───────────────────────────────────────────┬─────────────────────────────────┘
                                            │
┌───────────────────────────────────────────▼─────────────────────────────────┐
│ tencent_cloud_chat_sdk (not replaced, still the original package) │
│ - TencentCloudChatSdkPlatform (interface) │
│ - TIMConversationManager / TIMMessageManager etc. adapter │
│ - All APIs ultimately call platform.xxx() or NativeLibraryManager (FFI) │
└───────────────────────────────────────────┬─────────────────────────────────┘
                    │                                    │
        platform interface call FFI / Native call
                    │                                    │
┌───────────────────▼──────────────────┐   ┌────────────▼─────────────────────┐
│  Tim2ToxSdkPlatform（Dart）           │   │  tim2tox_ffi / dart_compat（C++）  │
│ - Implement TencentCloudChatSdkPlatform │ │ - SafeGetV2TIMManager() │
│ - addConversationListener etc. │ │ - manager->GetConversationManager│
│ - _conversationListeners etc. │ │ - conv_manager->AddConversation │
│ - globalCallback distribution (instance_id) │ │ - SendCallbackToDart(globalCallback)│
└───────────────────┬──────────────────┘   └────────────┬─────────────────────┘
                    │                                    │
                    │         Dart_PostCObject_DL        │
                    └────────────────┬───────────────────┘
                                     │
┌────────────────────────────────────▼─────────────────────────────────────────┐
│ Native layer (C++) │
│ - V2TIMManagerImpl (one for each instance, distinguished by instance_id when there are multiple instances) │
│ - V2TIMConversationManagerImpl::GetInstance() (singleton, but SetManagerImpl(this)) │
│ - V2TIMSignalingManagerImpl (one per instance, GetSignalingManager() independent per instance) │
│  - ToxManager / toxcore                                                       │
└─────────────────────────────────────────────────────────────────────────────┘
```

- **Binary replacement**: Do not replace classes of tencent_cloud_chat_sdk, only replace **Platform instance** and **Native implementation**.  
- **Tim2ToxSdkPlatform**: implements the `TencentCloudChatSdkPlatform` interface, and all "platform methods" (such as `addConversationListener`, `getConversationList`) are implemented by it; at the same time, it processes `globalCallback` from Native and distributes it to each listener according to `instance_id`.  
- **V2TIM***: Business implementation in C++ (session, message, group, signaling, etc.). Dart calls Native through FFI, and Native uses `V2TIMManagerImpl`, `GetConversationManager()`, etc. internally; the callback sends `globalCallback` to Dart through dart_compat, which is distributed by instance by **Tim2ToxSdkPlatform**.

## 2. Correspondence between Tim2ToxSdkPlatform and V2TIM*

| Capabilities | Dart side (Platform/Adapter) | C++ side (V2TIM*) |
|----------------|---------------------------------------------|------------------|
| Conversation List | Platform.getConversationList/provider | V2TIMConversationManagerImpl(GetConversationList, Cache, Pin) |
| Session listening | Platform.addConversationListener | V2TIMConversationManagerImpl::AddConversationListener + dart_compat registration DartConversationListenerImpl |
| Session change callback | Platform._conversationListeners + globalCallback distribution | C++ call listeners → dart_compat → SendCallbackToDart(globalCallback) |
| Message | Platform + FFI send/poll | V2TIMMessageManagerImpl + Tox |
| Group | Platform + FFI | V2TIMGroupManagerImpl |
| Signaling | Platform + FFI | V2TIMSignalingManagerImpl (per instance) |

- **Session Monitoring**:
  - **C++**: `V2TIMConversationManagerImpl` is a single instance, but every time `GetConversationManager()` will be `SetManagerImpl(this)`, the current instance will be used for subsequent operations. dart_compat uses the manager of the current instance to get `conv_manager`, and then `AddConversationListener(DartConversationListenerImpl)`; these listeners are notified when the C++ session changes, and then sent to Dart through globalCallback.  
  - **Dart**: `Tim2ToxSdkPlatform.addConversationListener` saves listeners to `_conversationListeners` (and the list by instance); when receiving globalCallback(ConversationEvent), Platform distributes them to these listeners by instance_id.  
- So: **The "source" of session listening is C++ V2TIMConversationManagerImpl + dart_compat; the "receiver and dispatch" are in Dart Tim2ToxSdkPlatform. **

## 3. addConversationListener implements position evaluation

### 3.1 "Implement" AddConversationListener in V2TIMConversationManagerImpl?

- **Status quo**: C++ implemented. `AddConversationListener` just puts `V2TIMConversationListener*` into `listeners_`; every time dart_compat sets various Conv callbacks on the Dart side, `GetOrCreateConversationListener()` will get a `DartConversationListenerImpl` and `conv_manager->AddConversationListener(listener)`.  
- **Conclusion**: The C++ side no longer needs to "implement" AddConversationListener, it is satisfied: C++ session changes → notify listeners → dart_compat sends globalCallback → Dart distribution.

### 3.2 Implement addConversationListener in Tim2ToxSdkPlatform?

- **Current situation**: The Platform interface requires the implementation of `addConversationListener`; Tim2ToxSdkPlatform **has been implemented**: put the listener into `_conversationListeners` and `_instanceConversationListeners[id]`.  
- **Problem**: The **root cause** of the test error "addConversationListener() has not been implemented" is the **calling timing**:
  - `_setupInternalConversationListener()` is called in the **constructor** of `Tim2ToxSdkPlatform`.  
  - Which will execute `TIMConversationManager.instance.addConversationListener(listener: internalListener)`.  
  - The adapter will adjust `TencentCloudChatSdkPlatform.instance.addConversationListener(...)` again.  
  - At this time, the constructor has not yet returned, `TencentCloudChatSdkPlatform.instance` has not been assigned to the current `Tim2ToxSdkPlatform`, it is still the default platform (or old instance), and its default implementation directly throws UnimplementedError.  
- **Conclusion**: The fix should be on the **Tim2ToxSdkPlatform** side, rather than implementing another layer in C++. Two options are available:
  1. **No longer synchronize internalListener to adapter in the constructor**: only keep `this.addConversationListener(listener: internalListener)` and do not call `TIMConversationManager.instance.addConversationListener(internalListener)` (internal only needs to be in _conversationListeners of Platform for globalCallback distribution).  
  2. **Delay synchronization**: Use `Future.microtask(() => TIMConversationManager.instance.addConversationListener(listener: internalListener))` and execute it after assigning `instance = Tim2ToxSdkPlatform(...)`, so that the adapter is transferred to the addConversationListener of the current Platform.

### 3.3 Recommended- **Implementation location**: Keep and use the addConversationListener implementation of **Tim2ToxSdkPlatform**; on the C++ side, **V2TIMConversationManagerImpl::AddConversationListener** can just keep the status quo.
- **Fix point**: In `_setupInternalConversationListener` of Tim2ToxSdkPlatform, avoid calling platform.addConversationListener through adapter when "instance has not been assigned a value". Recommended practice: **Only call `this.addConversationListener(listener: internalListener)`, delete or postpone the call to `TIMConversationManager.instance.addConversationListener(internalListener)`** (if it needs to be consistent with the adapter list, you can use the above microtask to postpone).

## 4. Summary

- **Binary replacement**: Tim2ToxSdkPlatform is "platform implementation", V2TIM* is "Native capability implementation"; the two collaborate through FFI + globalCallback, and Platform is responsible for distributing back to Dart listener by instance_id.  
- **Session Listening**: C++ has AddConversationListener and dart_compat registered; the Dart side is implemented by Tim2ToxSdkPlatform and holds the listener list, and distributes it when receiving globalCallback.  
- **addConversationListener is not implemented. Error**: comes from "when the platform is called through the adapter in the constructor, the instance has not yet pointed to the current Platform"; it should be solved by adjusting the calling sequence or delaying synchronization in **Tim2ToxSdkPlatform** instead of implementing another layer in V2TIMConversationManagerImpl.