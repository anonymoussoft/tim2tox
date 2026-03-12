# 构建说明

## 快速开始

### 构建 FFI 库（智能构建）

```bash
cd tim2tox
./build_ffi.sh
```

这个脚本会：
- ✅ 仅在需要时构建（库不存在或源文件更新）
- ✅ 自动配置 CMake（如果需要）
- ✅ 验证构建结果

### 运行测试（自动构建）

```bash
cd auto_tests
./run_tests.sh
```

测试脚本会自动检查并构建库（如果需要）。

## 构建选项

### 1. 智能构建（推荐）

```bash
./build_ffi.sh
```

**优点**:
- 仅在需要时构建
- 节省时间
- 自动检查依赖

### 2. 完整构建

```bash
bash build.sh
```

**用途**:
- 首次构建
- 清理后重建
- 构建所有目标

### 3. 强制重建

```bash
rm -rf build
./build_ffi.sh
```

## 构建检查逻辑

脚本会检查以下条件：

1. **库文件不存在** → 构建
2. **源文件更新** → 重建
3. **CMakeLists.txt 更新** → 重建
4. **构建未配置** → 配置并构建
5. **其他情况** → 跳过构建

## 验证构建

构建完成后，验证库文件：

```bash
# 检查库文件
ls -la build/ffi/libtim2tox_ffi.dylib

# 验证符号
nm -g build/ffi/libtim2tox_ffi.dylib | grep Dart_PostCObject_DL
```

## 常见问题

### Q: 如何强制重建？
A: 删除库文件或构建目录：
```bash
rm -f build/ffi/libtim2tox_ffi.dylib
./build_ffi.sh
```

### Q: 构建失败怎么办？
A: 清理并重新构建：
```bash
rm -rf build
./build_ffi.sh
```

### Q: 如何只构建 FFI 库？
A: 使用智能构建脚本：
```bash
./build_ffi.sh
```

### Q: 如何查看构建日志？
A: 构建输出会显示在终端，或查看：
```bash
tail -f build.log  # 如果使用 build.sh
```

## 性能

- **首次构建**: 5-15 分钟（完整构建）
- **增量构建**: 1-5 分钟（仅更新文件）
- **跳过构建**: < 1 秒（无更改）

## 相关文件

- `build_ffi.sh` - 智能构建脚本
- `build.sh` - 完整构建脚本
- `auto_tests/run_tests.sh` - 测试脚本（自动构建）
- `auto_tests/run_tests_with_lib.sh` - 带库路径的测试脚本
