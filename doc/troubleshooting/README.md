# Tim2Tox 排障与历史调查索引

本文汇总 Tim2Tox 的“排障入口”“测试失败定位入口”和“历史调查记录”，避免和主线架构/接入文档混在一起。

## 先从哪里开始

- **我在跑自动化测试，遇到失败/卡住/不稳定**：先读 [auto_tests/README.md](../../auto_tests/README.md)，再按 README 中的“故障排除/最佳实践”逐步定位。
- **我遇到 Native 崩溃（FFI、符号、回调、线程）**：先读 [auto_tests/NATIVE_CRASH_COMMON_ISSUES.md](../../auto_tests/NATIVE_CRASH_COMMON_ISSUES.md)，需要看 native 栈时再读 [auto_tests/DEBUG_NATIVE_CRASH.md](../../auto_tests/DEBUG_NATIVE_CRASH.md)。
- **我需要了解某个历史问题的机制分析**：见下方“历史调查记录”（这些文档通常不保证仍适用于当前版本）。

## 测试与排障文档（推荐入口）

- [auto_tests/README.md](../../auto_tests/README.md) — 测试套件总入口（如何运行、覆盖范围、失败记录、故障排除）
- [auto_tests/FAILURE_RECORDS.md](../../auto_tests/FAILURE_RECORDS.md) — 失败记录与修复状态
- [auto_tests/NATIVE_CRASH_COMMON_ISSUES.md](../../auto_tests/NATIVE_CRASH_COMMON_ISSUES.md) — Native 崩溃常见问题速查
- [auto_tests/DEBUG_NATIVE_CRASH.md](../../auto_tests/DEBUG_NATIVE_CRASH.md) — 用 lldb 查看 native 栈