# Tim2Tox 文档
> 语言 / Language: [中文](README.md) | [English](README.en.md)


## 维护入口

- [ARCHITECTURE.md](ARCHITECTURE.md) - 整体架构、分层职责、数据流与设计决策
- [API_REFERENCE.md](API_REFERENCE.md) - V2TIM、FFI 与 Dart 层 API 参考
- [DEVELOPMENT_GUIDE.md](DEVELOPMENT_GUIDE.md) - 开发流程、构建、测试与调试建议
- [MODULARIZATION.md](MODULARIZATION.md) - FFI 模块拆分结构与模块职责

## 核心机制

- [BINARY_REPLACEMENT.md](BINARY_REPLACEMENT.md) - 动态库替换方案与调用链路
- [FFI_COMPAT_LAYER.md](FFI_COMPAT_LAYER.md) - Dart 兼容层回调机制、JSON 格式与实现状态
- [RESTORE_AND_PERSISTENCE.md](RESTORE_AND_PERSISTENCE.md) - 持久化与恢复流程
- [TOXAV_AND_SIGNALING.md](TOXAV_AND_SIGNALING.md) - ToxAV、signaling、TUICallKit 与实例路由
- [BOOTSTRAP_AND_POLLING.md](BOOTSTRAP_AND_POLLING.md) - Bootstrap 节点加载、联网建立与 poll loop

## 兼容性与专题

- [MULTI_INSTANCE_SUPPORT.md](MULTI_INSTANCE_SUPPORT.md) - 多实例支持场景与 API
- [FFI_FUNCTION_DECLARATION_GUIDE.md](FFI_FUNCTION_DECLARATION_GUIDE.md) - FFI 函数声明规则与自检清单
- [ISCUSTOMPLATFORM_ROUTING_IMPACT.md](ISCUSTOMPLATFORM_ROUTING_IMPACT.md) - `isCustomPlatform` 路由统一影响评估
- [PLATFORM_VS_V2TIM_AND_CONVERSATION_LISTENER.md](PLATFORM_VS_V2TIM_AND_CONVERSATION_LISTENER.md) - 二进制替换下 Platform 与 V2TIM 关系分析

## 客户端联动

- [主 README](../README.md)
- [toxee 文档索引](../../toxee/doc/README.md)
- [toxee 账号与会话](../../toxee/doc/ACCOUNT_AND_SESSION.md)
