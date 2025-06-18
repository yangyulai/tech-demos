@echo off
chcp 65001 >nul
echo 编译日志收集系统...

REM 编译服务端
echo 编译服务端...
cl /EHsc /std:c++17 LogCollectorServer.cpp /Fe:LogServer.exe
if errorlevel 1 (
    echo 服务端编译失败！
    pause
    exit /b 1
)

REM 编译客户端
echo 编译客户端...
cl /EHsc /std:c++17 LogClient.cpp /Fe:LogClient.exe
if errorlevel 1 (
    echo 客户端编译失败！
    pause
    exit /b 1
)

echo.
echo 编译完成！
echo.
echo 使用说明：
echo 1. 先运行 LogServer.exe 启动日志服务器
echo 2. 然后运行一个或多个 LogClient.exe 实例
echo 3. 日志将保存在 collected_logs.txt 文件中
echo.
pause