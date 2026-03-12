# Tim2Tox Examples

这个目录包含了使用 tim2tox 库的示例程序，演示如何创建 Tox 客户端和服务器。

## 文件说明

- `echo_bot_server.c` - Echo Bot 服务器，基于 Tox Wiki 示例
- `tim2tox_client.cpp` - 使用 tim2tox 库的客户端示例
- `CMakeLists.txt` - CMake 构建配置
- `build_examples.sh` - 构建脚本

## 构建要求

- CMake 3.10+
- libsodium
- libtoxcore
- C++17 编译器

## 构建步骤

1. 确保已经构建了主项目：
   ```bash
   cd ..
   ./build.sh
   ```

2. 构建示例：
   ```bash
   cd example
   ./build_examples.sh
   ```

## 运行示例

### 1. 启动 Echo Bot 服务器

```bash
cd example/build
./echo_bot_server
```

服务器启动后会显示其 Tox ID，类似：
```
=== Echo Bot Server ===
Tox ID: F404ABAA1C99A9D37D61AB54898F56793E1DEF8BD46B1038B9D822E8460FAB67
Status: Echoing your messages
=======================
Server: Online (UDP)
```

### 2. 运行 Tim2Tox 客户端

在另一个终端中：
```bash
cd example/build
./tim2tox_client F404ABAA1C99A9D37D61AB54898F56793E1DEF8BD46B1038B9D822E8460FAB67
```

### 3. 测试消息收发

客户端启动后，可以使用以下命令：

- `connect` - 连接到服务器
- `send <message>` - 发送消息
- `status` - 查看连接状态
- `help` - 显示帮助信息
- `quit` - 退出客户端

或者直接输入消息内容发送。

## 测试流程

1. 启动服务器并记录 Tox ID
2. 启动客户端，使用服务器的 Tox ID
3. 在客户端输入 `connect` 发送好友请求
4. 服务器会自动接受好友请求
5. 发送消息测试回显功能

## 预期结果

- 客户端发送的消息会被服务器回显
- 连接状态会实时显示
- 好友关系会自动建立

## 故障排除

1. **构建失败**: 确保已安装 libsodium 和 libtoxcore
2. **连接失败**: 检查网络连接和防火墙设置
3. **消息发送失败**: 确保好友关系已建立

## 扩展功能

可以基于这些示例开发更复杂的功能：

- 群组聊天
- 文件传输
- 音视频通话
- 自定义消息类型 