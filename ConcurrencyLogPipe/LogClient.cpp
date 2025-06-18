// LogClient.cpp - 日志发送客户端
#include <windows.h>
#include <iostream>
#include <string>
#include <sstream>
#include <thread>
#include <chrono>
#include <vector>
class LogClient {
private:
    static constexpr LPCWSTR PIPE_NAME = L"\\\\.\\pipe\\LogCollectorPipe";
    static constexpr int BUFFER_SIZE = 4096;
    
    HANDLE hPipe;
    std::string processName;
    DWORD processId;
    
public:
    LogClient(const std::string& name = "") 
        : hPipe(INVALID_HANDLE_VALUE), processName(name) {
        processId = GetCurrentProcessId();
        if (processName.empty()) {
            processName = "Process_" + std::to_string(processId);
        }
    }
    
    ~LogClient() {
        disconnect();
    }
    
    bool connect() {
        // 等待管道可用
        while (true) {
            hPipe = CreateFile(
                PIPE_NAME,
                GENERIC_WRITE,
                0,
                NULL,
                OPEN_EXISTING,
                0,
                NULL
            );
            
            if (hPipe != INVALID_HANDLE_VALUE) {
                break;
            }
            
            // 如果管道忙，等待
            if (GetLastError() != ERROR_PIPE_BUSY) {
                std::cerr << "无法连接到日志服务器，错误码: " << GetLastError() << std::endl;
                return false;
            }
            
            // 等待管道可用（最多等待20秒）
            if (!WaitNamedPipe(PIPE_NAME, 20000)) {
                std::cerr << "等待管道超时" << std::endl;
                return false;
            }
        }
        
        // 设置管道为消息模式
        DWORD mode = PIPE_READMODE_MESSAGE;
        BOOL success = SetNamedPipeHandleState(hPipe, &mode, NULL, NULL);
        if (!success) {
            std::cerr << "设置管道模式失败" << std::endl;
            CloseHandle(hPipe);
            hPipe = INVALID_HANDLE_VALUE;
            return false;
        }
        
        std::cout << "已连接到日志服务器" << std::endl;
        return true;
    }
    
    void disconnect() {
        if (hPipe != INVALID_HANDLE_VALUE) {
            CloseHandle(hPipe);
            hPipe = INVALID_HANDLE_VALUE;
        }
    }
    
    bool sendLog(const std::string& level, const std::string& message) {
        if (hPipe == INVALID_HANDLE_VALUE) {
            if (!connect()) {
                return false;
            }
        }
        
        // 构造日志消息
        std::stringstream ss;
        ss << "[" << processName << "][PID:" << processId << "][" << level << "] " << message;
        std::string logMessage = ss.str();
        
        // 发送日志
        DWORD bytesWritten;
        BOOL success = WriteFile(
            hPipe,
            logMessage.c_str(),
            (DWORD)logMessage.length(),
            &bytesWritten,
            NULL
        );
        
        if (!success) {
            std::cerr << "发送日志失败，错误码: " << GetLastError() << std::endl;
            disconnect();
            return false;
        }
        
        // 等待服务器确认
        char ackBuffer[10];
        DWORD bytesRead;
        success = ReadFile(hPipe, ackBuffer, sizeof(ackBuffer), &bytesRead, NULL);
        
        if (!success) {
            std::cerr << "接收确认失败" << std::endl;
            disconnect();
            return false;
        }
        
        return true;
    }
    
    // 便捷日志方法
    void logInfo(const std::string& message) {
        sendLog("INFO", message);
    }
    
    void logWarning(const std::string& message) {
        sendLog("WARN", message);
    }
    
    void logError(const std::string& message) {
        sendLog("ERROR", message);
    }
    
    void logDebug(const std::string& message) {
        sendLog("DEBUG", message);
    }
};

// 示例：模拟多进程发送日志
void simulateProcess(int id) {
    LogClient logger("Worker_" + std::to_string(id));
    
    if (!logger.connect()) {
        std::cerr << "进程 " << id << " 连接失败" << std::endl;
        return;
    }
    
    // 发送多条日志消息
    for (int i = 0; i < 10; i++) {
        std::string message = "这是来自工作进程 " + std::to_string(id) + 
                            " 的第 " + std::to_string(i + 1) + " 条消息";
        
        switch (i % 4) {
            case 0:
                logger.logInfo(message);
                break;
            case 1:
                logger.logDebug(message);
                break;
            case 2:
                logger.logWarning(message);
                break;
            case 3:
                logger.logError(message);
                break;
        }
        
        // 模拟处理延迟
        std::this_thread::sleep_for(std::chrono::milliseconds(100 + rand() % 200));
    }
    
    std::cout << "进程 " << id << " 完成日志发送" << std::endl;
}

int main() {
    // 设置控制台编码为UTF-8
    SetConsoleOutputCP(CP_UTF8);
    
    std::cout << "日志客户端示例" << std::endl;
    std::cout << "1. 单进程模式" << std::endl;
    std::cout << "2. 多线程模拟多进程" << std::endl;
    std::cout << "请选择模式: ";
    
    int choice;
    std::cin >> choice;
    std::cin.ignore();
    
    if (choice == 1) {
        // 单进程模式
        LogClient logger("MainProcess");
        
        if (!logger.connect()) {
            std::cerr << "连接日志服务器失败" << std::endl;
            return 1;
        }
        
        std::cout << "已连接到日志服务器，输入日志消息（输入 'quit' 退出）：" << std::endl;
        
        std::string input;
        while (std::getline(std::cin, input)) {
            if (input == "quit") {
                break;
            }
            
            logger.logInfo(input);
        }
    } else if (choice == 2) {
        // 多线程模拟多进程
        const int NUM_PROCESSES = 5;
        std::vector<std::thread> threads;
        
        std::cout << "启动 " << NUM_PROCESSES << " 个模拟进程..." << std::endl;
        
        for (int i = 0; i < NUM_PROCESSES; i++) {
            threads.emplace_back(simulateProcess, i + 1);
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
        
        // 等待所有线程完成
        for (auto& t : threads) {
            t.join();
        }
        
        std::cout << "所有进程已完成" << std::endl;
    }
    
    return 0;
}