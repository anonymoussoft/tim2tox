# Tim2Tox

Tim2Tox 是一个基于 c-toxcore 的 Tox 客户端库，提供了简单的 API 来创建 Tox 应用程序。

## 特性

- 纯 C++ 实现
- 静态库构建
- 不依赖音视频功能
- 支持 IPv6
- 完整的错误处理和日志系统

## 依赖

- CMake >= 3.4.1
- C++20 兼容的编译器
- [c-toxcore](https://github.com/TokTok/c-toxcore)
- libsodium

## 构建

### 使用脚本构建

```bash
./build.sh
```

### 手动构建

```bash
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

### CMake 选项

主要的构建选项：

- `BUILD_TOXAV`: 是否构建音视频支持（默认：OFF）
- `USE_IPV6`: 启用 IPv6 支持（默认：ON）
- `ENABLE_STATIC`: 构建静态库（默认：ON）
- `ENABLE_SHARED`: 构建动态库（默认：OFF）

日志级别选项：

- `ERROR`: 启用错误日志（默认：ON）
- `WARNING`: 启用警告日志（默认：ON）
- `INFO`: 启用信息日志（默认：ON）
- `DEBUG`: 启用调试日志（默认：OFF）
- `TRACE`: 启用跟踪日志（默认：OFF）

## 使用

### 包含头文件

```cpp
#include <tim2tox/ToxManager.h>
```

### 基本用法

```cpp
#include <tim2tox/ToxManager.h>

int main() {
    ToxManager manager;
    // 初始化 Tox
    if (!manager.init()) {
        return 1;
    }
    
    // 开始运行
    manager.run();
    return 0;
}
```

## 许可证

本项目采用 GPL-3.0 许可证。详见 [LICENSE](LICENSE) 文件。 