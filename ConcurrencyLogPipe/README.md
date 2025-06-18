# Windows高并发日志收集系统

基于Windows命名管道的高性能日志收集系统，支持多进程并发日志传输。

## 系统要求

- Windows 7 或更高版本
- CMake 3.10 或更高版本
- 支持C++17的编译器：
  - Visual Studio 2017 或更高版本
  - MinGW-w64 7.0 或更高版本
  - Clang

## 快速开始

### 方法1：使用构建脚本（推荐）

```batch
build_cmake.bat
```

脚本会引导您选择构建类型和编译器。

### 方法2：手动使用CMake

```batch
# 创建构建目录
mkdir build
cd build

# 配置项目（Visual Studio）
cmake -G "Visual Studio 17 2022" -A x64 ..

# 或使用其他生成器
cmake -G "Ninja" -DCMAKE_BUILD_TYPE=Release ..

# 编译
cmake --build . --config Release
```

### 方法3：使用CMake GUI

1. 打开CMake GUI
2. 设置源代码目录和构建目录
3. 点击 Configure，选择生成器
4. 点击 Generate
5. 打开生成的项目文件或使用命令行构建

## 构建选项

CMakeLists.txt 支持以下选项：

- `CMAKE_BUILD_TYPE`: Debug, Release, RelWithDebInfo, MinSizeRel
- `CMAKE_INSTALL_PREFIX`: 安装目录（默认：C:/Program Files/LogCollectorSystem）

## 目录结构

```
project/
├── CMakeLists.txt          # CMake配置文件
├── LogCollectorServer.cpp  # 服务端源码
├── LogClient.cpp          # 客户端源码
├── build_cmake.bat        # 构建脚本
├── README.md              # 本文件
└── build/                 # 构建目录（自动生成）
    ├── bin/              # 可执行文件
    ├── StartServer.bat   # 启动服务器脚本
    ├── StartClient.bat   # 启动客户端脚本
    └── StartMultipleClients.bat  # 启动多客户端脚本
```

## 运行系统

1. **启动服务器**
   ```batch
   cd build
   StartServer.bat
   ```
   或直接运行：
   ```batch
   build\bin\LogServer.exe
   ```

2. **启动客户端**
   ```batch
   cd build
   StartClient.bat
   ```
   或直接运行：
   ```batch
   build\bin\LogClient.exe
   ```

3. **启动多个客户端**
   ```batch
   cd build
   StartMultipleClients.bat
   ```

## 功能特性

- ✅ 高并发支持（最多100个并发连接）
- ✅ 多线程处理
- ✅ 线程安全的日志写入
- ✅ 自动时间戳
- ✅ 多日志级别（INFO, DEBUG, WARNING, ERROR）
- ✅ 进程标识（进程名 + PID）
- ✅ 自动重连机制
- ✅ 消息确认机制

## 性能优化

1. **调整并发连接数**：修改 `MAX_INSTANCES` 常量
2. **调整缓冲区大小**：修改 `BUFFER_SIZE` 常量
3. **使用Release模式**：获得最佳性能

## 故障排除

### 编译错误

1. **找不到CMake**
   - 下载安装：https://cmake.org/download/
   - 确保CMake在系统PATH中

2. **C++17支持问题**
   - 更新编译器到支持C++17的版本
   - Visual Studio 2017更新到最新版本

3. **链接错误**
   - 确保Windows SDK已正确安装

### 运行时错误

1. **无法创建命名管道**
   - 检查是否有权限
   - 检查防火墙设置

2. **客户端连接失败**
   - 确保服务器已启动
   - 检查是否达到最大连接数

## 扩展开发

### 添加新功能

1. 修改源代码
2. 重新运行 `build_cmake.bat`
3. 测试新功能

### 集成到其他项目

```cmake
# 在您的CMakeLists.txt中
add_subdirectory(log_collector)
target_link_libraries(your_app PRIVATE LogClient)
```

## 许可证

本项目使用 MIT 许可证。