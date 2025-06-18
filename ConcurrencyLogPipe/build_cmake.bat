@echo off
chcp 65001 >nul
setlocal enabledelayedexpansion

echo ========================================
echo     日志收集系统 CMake 构建脚本
echo ========================================
echo.

REM 检查CMake是否安装
where cmake >nul 2>&1
if errorlevel 1 (
    echo 错误：未找到CMake，请先安装CMake
    echo 下载地址：https://cmake.org/download/
    pause
    exit /b 1
)

REM 获取脚本所在目录
set "SCRIPT_DIR=%~dp0"
cd /d "%SCRIPT_DIR%"

REM 选择构建类型
echo 请选择构建类型：
echo 1. Debug（调试版本）
echo 2. Release（发布版本）
echo 3. RelWithDebInfo（带调试信息的发布版本）
echo 4. MinSizeRel（最小体积发布版本）
set /p BUILD_TYPE_CHOICE="请输入选择 (1-4) [默认: 2]: "

if "%BUILD_TYPE_CHOICE%"=="" set BUILD_TYPE_CHOICE=2

if "%BUILD_TYPE_CHOICE%"=="1" (
    set BUILD_TYPE=Debug
) else if "%BUILD_TYPE_CHOICE%"=="2" (
    set BUILD_TYPE=Release
) else if "%BUILD_TYPE_CHOICE%"=="3" (
    set BUILD_TYPE=RelWithDebInfo
) else if "%BUILD_TYPE_CHOICE%"=="4" (
    set BUILD_TYPE=MinSizeRel
) else (
    echo 无效的选择，使用默认Release模式
    set BUILD_TYPE=Release
)

echo.
echo 选择的构建类型：%BUILD_TYPE%
echo.

REM 选择生成器
echo 请选择CMake生成器：
echo 1. Visual Studio（自动检测版本）
echo 2. NMake Makefiles
echo 3. Ninja
echo 4. MinGW Makefiles
set /p GENERATOR_CHOICE="请输入选择 (1-4) [默认: 1]: "

if "%GENERATOR_CHOICE%"=="" set GENERATOR_CHOICE=1
echo %ProgramFiles%
if "%GENERATOR_CHOICE%"=="1" (
    REM 自动检测Visual Studio版本
    if exist "%ProgramFiles%\Microsoft Visual Studio\2022" (
        set CMAKE_GENERATOR="Visual Studio 17 2022"
        set CMAKE_ARCH=-A x64
    ) else if exist "%ProgramFiles(x86)%\Microsoft Visual Studio\2019" (
        set CMAKE_GENERATOR="Visual Studio 16 2019"
        set CMAKE_ARCH=-A x64
    ) else if exist "%ProgramFiles(x86)%\Microsoft Visual Studio\2017" (
        set CMAKE_GENERATOR="Visual Studio 15 2017 Win64"
        set CMAKE_ARCH=
    ) else (
        echo 错误：未找到Visual Studio安装
        pause
        exit /b 1
    )
    set NEED_BUILD=1
) else if "%GENERATOR_CHOICE%"=="2" (
    set CMAKE_GENERATOR="NMake Makefiles"
    set CMAKE_ARCH=
    set NEED_BUILD=0
) else if "%GENERATOR_CHOICE%"=="3" (
    set CMAKE_GENERATOR="Ninja"
    set CMAKE_ARCH=
    set NEED_BUILD=0
) else if "%GENERATOR_CHOICE%"=="4" (
    set CMAKE_GENERATOR="MinGW Makefiles"
    set CMAKE_ARCH=
    set NEED_BUILD=0
) else (
    echo 无效的选择，使用默认Visual Studio
    set CMAKE_GENERATOR="Visual Studio 17 2022"
    set CMAKE_ARCH=-A x64
    set NEED_BUILD=1
)

echo.
echo 使用生成器：%CMAKE_GENERATOR%
echo.

REM 创建构建目录
set BUILD_DIR=build-%BUILD_TYPE%
if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"

REM 进入构建目录
cd "%BUILD_DIR%"

REM 运行CMake配置
echo 正在配置项目...
cmake -G %CMAKE_GENERATOR% %CMAKE_ARCH% -DCMAKE_BUILD_TYPE=%BUILD_TYPE% ..
if errorlevel 1 (
    echo CMake配置失败！
    cd ..
    pause
    exit /b 1
)

echo.
echo 正在编译项目...

REM 根据生成器选择构建命令
if "%NEED_BUILD%"=="1" (
    REM Visual Studio项目
    cmake --build . --config %BUILD_TYPE%
) else (
    REM Makefile项目
    cmake --build .
)

if errorlevel 1 (
    echo 编译失败！
    cd ..
    pause
    exit /b 1
)

echo.
echo ========================================
echo          编译成功完成！
echo ========================================
echo.
echo 可执行文件位置：%CD%\bin
echo.
echo 运行说明：
echo 1. 运行 StartServer.bat 启动服务器
echo 2. 运行 StartClient.bat 启动单个客户端
echo 3. 运行 StartMultipleClients.bat 启动多个客户端
echo.

REM 返回原目录
cd ..

REM 询问是否运行
set /p RUN_NOW="是否立即运行服务器？(Y/N) [默认: N]: "
if /i "%RUN_NOW%"=="Y" (
    echo.
    echo 启动服务器...
    start "Log Server" /D "%BUILD_DIR%\bin" LogServer.exe
    
    timeout /t 2 >nul
    
    set /p RUN_CLIENT="是否运行客户端？(Y/N) [默认: Y]: "
    if /i "!RUN_CLIENT!"=="Y" (
        start "Log Client" /D "%BUILD_DIR%\bin" LogClient.exe
    )
)

pause