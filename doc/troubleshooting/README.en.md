# Tim2Tox Troubleshooting & Historical Investigations Index

This page consolidates Tim2Tox troubleshooting entry points, test-failure navigation, and historical investigation notes, keeping them separate from the main architecture/integration docs.

## Where to start

- **I’m running the auto tests and something fails / hangs / flakes**: start with [auto_tests/README.md](../../auto_tests/README.md), then follow the “troubleshooting / best practices” sections there.
- **I hit a native crash (FFI, symbols, callbacks, threading)**: start with [auto_tests/NATIVE_CRASH_COMMON_ISSUES.md](../../auto_tests/NATIVE_CRASH_COMMON_ISSUES.md), then use [auto_tests/DEBUG_NATIVE_CRASH.md](../../auto_tests/DEBUG_NATIVE_CRASH.md) to capture native stacks via lldb if needed.
- **I want mechanism-level analysis of a past issue**: see “Historical investigations” below (not guaranteed to match current versions).

## Test & troubleshooting docs (recommended entry points)

- [auto_tests/README.md](../../auto_tests/README.md) — test suite hub (how to run, coverage, failure records, troubleshooting)
- [auto_tests/FAILURE_RECORDS.md](../../auto_tests/FAILURE_RECORDS.md) — failure records and fix status
- [auto_tests/NATIVE_CRASH_COMMON_ISSUES.md](../../auto_tests/NATIVE_CRASH_COMMON_ISSUES.md) — common native crash issues (quick check)
- [auto_tests/DEBUG_NATIVE_CRASH.md](../../auto_tests/DEBUG_NATIVE_CRASH.md) — inspect native stacks via lldb