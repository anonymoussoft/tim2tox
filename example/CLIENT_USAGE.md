# Echo Bot 测试客户端使用说明

## 概述

`echo_bot_client.c` 是一个简单的 Tox 客户端，用于测试 `echo_bot_server.c` 的功能。客户端可以连接到 echo bot 服务器，发送消息并接收回显。

## 编译

```bash
cd tim2tox/example
mkdir -p build
cd build
cmake ..
make
```

这将生成两个可执行文件：
- `echo_bot_server` - Echo Bot 服务器
- `echo_bot_client` - Echo Bot 测试客户端

## 使用方法

### 1. 启动 Echo Bot 服务器

```bash
./echo_bot_server
```

服务器启动后会显示：
```
=== Echo Bot Server ===
Tox ID: [服务器的 Tox ID]
Status: Echoing your messages
=======================
Server starting...
Press Ctrl+C to stop
```

**重要：** 记录下服务器显示的 Tox ID，客户端需要用它来连接。

### 2. 启动测试客户端

```bash
./echo_bot_client
```

客户端启动后会显示：
```
=== Echo Bot Client ===
Tox ID: [客户端的 Tox ID]
Status: Ready to test echo server
=======================
Client starting...

=== Commands ===
add <tox_id> - Add friend by Tox ID
send <message> - Send message to friend
status - Show connection status
friends - List friends
help - Show this help
quit/exit - Exit client
=================
```

### 3. 连接和测试

#### 步骤 1：添加服务器为好友
```
add [服务器的 Tox ID]
```

例如：
```
add F404ABAA1C99A9D37D61AB54898F56793E1DEF8BD46B1038B9D822E8460FAB67
```

#### 步骤 2：等待连接建立
客户端会自动显示连接状态：
```
Client: Online (UDP)
Friend request sent to Tox ID: [服务器 Tox ID] (friend number: 0)
Friend 0 is online (UDP)
Auto-selected friend 0 for messaging
```

#### 步骤 3：发送测试消息
```
Hello World!
```

或者使用 send 命令：
```
send Hello World!
```

#### 步骤 4：查看回显
客户端会显示：
```
Message sent to friend 0: Hello World! (message ID: 1)
Echo received from friend 0: Hello World!
```

### 4. 其他命令

- `status` - 查看连接状态和好友列表
- `friends` - 显示好友列表
- `help` - 显示帮助信息
- `quit` 或 `exit` - 退出客户端

## 测试场景

### 基本回显测试
1. 发送文本消息，验证是否收到相同的回显
2. 发送多行消息
3. 发送特殊字符和表情符号

### 连接测试
1. 测试 TCP 和 UDP 连接
2. 测试断线重连
3. 测试多个客户端同时连接

### 错误处理测试
1. 发送空消息
2. 发送超长消息
3. 网络中断恢复

## 故障排除

### 连接问题
- 确保服务器和客户端都连接到 Tox 网络
- 检查防火墙设置
- 验证 Tox ID 格式正确（64个十六进制字符）

### 消息问题
- 确保好友连接状态为在线
- 检查消息长度限制
- 验证网络连接稳定性

### 编译问题
- 确保已安装 libsodium
- 检查 CMake 和编译器版本
- 验证 toxcore 库路径正确

## 文件说明

- `echo_bot_server.c` - Echo Bot 服务器实现
- `echo_bot_client.c` - Echo Bot 测试客户端实现
- `echo_bot_savedata.tox` - 服务器保存数据文件
- `echo_bot_client_savedata.tox` - 客户端保存数据文件

## 注意事项

1. 首次运行需要时间连接到 Tox 网络
2. 保存数据文件用于保持 Tox ID 和好友列表
3. 服务器会自动接受所有好友请求
4. 客户端会自动选择第一个好友进行消息发送
5. 使用 Ctrl+C 安全退出程序
