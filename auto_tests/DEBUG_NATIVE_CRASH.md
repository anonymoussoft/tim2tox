# 在崩溃时查看 Native 栈和变量

当 `scenario_conversation_test` 在 native 层（libtim2tox_ffi.dylib）崩溃时，用 lldb 运行测试可以在崩溃停住时查看 native 调用栈和变量。

## 方式一：在终端用 lldb 跑测试（推荐）

在 **Terminal** 里运行脚本（崩溃时 lldb 会停住，可输入 `bt`、`frame variable` 等）：

```bash
cd tim2tox/auto_tests
chmod +x run_conversation_test_with_lldb.sh
./run_conversation_test_with_lldb.sh
```

脚本会：

1. 用 `lldb` 启动 `flutter test test/scenarios/scenario_conversation_test.dart`
2. 设置 `target.process.follow-fork-mode child`，让 lldb 跟到实际加载 dylib 的子进程（flutter_tester）
3. 执行 `run` 开始测试

**崩溃发生后**，lldb 会停住，例如：

```
Process XXXXX stopped
* thread #1, stop reason = signal SIGSEGV
    frame #0: 0x00000001xxxxxx libtim2tox_ffi.dylib`DartGetConversationListByFilter + 268
    ...
(lldb)
```

此时可输入：

- `bt`：查看 native 完整调用栈
- `frame variable`：当前帧的局部变量
- `frame select 0` 再 `frame variable`：查看某一帧的变量
- `dis`：当前帧反汇编

## 方式二：在 Xcode 里配成“自定义测试” Scheme

这样可以在 Xcode 里一键跑同一套命令，并在控制台看到 lldb 输出；**崩溃时的交互调试仍建议用方式一在终端里跑**。

### 步骤 1：建一个用来放 Scheme 的 Xcode 工程（若还没有）

1. 打开 Xcode，**File → New → Project**
2. 选 **macOS → App**，Next
3. Product Name 填：`FlutterTestRunner`（或任意）
4. 选一个目录（例如 `tim2tox/auto_tests/` 或项目根目录），Create

### 步骤 2：加一个专门跑 conversation 测试的 Scheme

1. 菜单 **Product → Scheme → New Scheme...**
2. Name 填：`Conversation Test (lldb)`
3. 在左侧选该 Scheme，点 **Edit...**（或双击 Scheme 名）

### 步骤 3：把 Run 配成用 lldb 跑 flutter test

1. 左侧选 **Run**
2. **Info** 里：
   - **Executable**：选 **Other...**，在弹窗里按 **Cmd+Shift+G**，输入 `/usr/bin/lldb`，选 `lldb`，Open
   - **Arguments**：点 **Arguments Passed** 下面的 **+**，依次添加（每行一条）：
     ```
     -o
     settings set target.process.follow-fork-mode child
     -o
     run
     --
     flutter
     test
     test/scenarios/scenario_conversation_test.dart
     ```
   - **Options** 里把 **Working Directory** 设为 `tim2tox/auto_tests` 的**绝对路径**（例如 `/Users/你的用户名/chat-uikit/tim2tox/auto_tests`）

3. 若本机 `flutter` 不在默认 PATH 里，再在 **Run → Arguments** 里勾选 **Environment Variables**，添加：
   - Name: `PATH`
   - Value: 你的 PATH（包含 flutter 的 bin 目录，例如 `/Users/你的用户名/flutter/bin:/usr/bin:/bin`）

4. 点 **Close** 保存

### 步骤 4：运行

1. 在 Scheme 下拉里选 **Conversation Test (lldb)**
2. **Product → Run**（或 Cmd+R）
3. 控制台会看到 lldb 和测试输出；若发生 native 崩溃，会看到类似 `Process ... stopped, signal SIGSEGV`。此时若要输入 `bt`、`frame variable` 等，**在终端里执行方式一的脚本**更合适，因为 Xcode 控制台对 lldb 的交互支持有限。

## 小结

| 目的                     | 做法                         |
|--------------------------|------------------------------|
| 崩溃时看 native 栈和变量 | 在终端运行 `./run_conversation_test_with_lldb.sh` |
| 在 Xcode 里一键跑同一测试 | 按上面步骤配好 “Conversation Test (lldb)” Scheme 后 Cmd+R |
