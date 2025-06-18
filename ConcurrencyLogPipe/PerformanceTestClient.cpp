// PerformanceTestClient.cpp - 性能测试客户端
#include <windows.h>
#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <atomic>
#include <random>
#include <sstream>

class PerformanceTestClient {
private:
    static constexpr LPCWSTR PIPE_NAME = L"\\\\.\\pipe\\LogCollectorPipe";
    static constexpr int BUFFER_SIZE = 4096;
    
    std::atomic<uint64_t> totalSent{0};
    std::atomic<uint64_t> totalFailed{0};
    std::atomic<bool> running{true};
    
    void clientWorker(int clientId, int messageCount, int delayMs) {
        HANDLE hPipe = INVALID_HANDLE_VALUE;
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, delayMs);
        
        // 连接到服务器
        while (running) {
            hPipe = CreateFile(
                PIPE_NAME,
                GENERIC_READ | GENERIC_WRITE,
                0,
                NULL,
                OPEN_EXISTING,
                0,
                NULL
            );
            
            if (hPipe != INVALID_HANDLE_VALUE) {
                break;
            }
            
            if (GetLastError() != ERROR_PIPE_BUSY) {
                std::cerr << "客户端 " << clientId << " 无法连接到服务器" << std::endl;
                return;
            }
            
            if (!WaitNamedPipe(PIPE_NAME, 5000)) {
                std::cerr << "客户端 " << clientId << " 等待超时" << std::endl;
                return;
            }
        }
        
        // 设置管道模式
        DWORD mode = PIPE_READMODE_MESSAGE;
        SetNamedPipeHandleState(hPipe, &mode, NULL, NULL);
        
        std::cout << "客户端 " << clientId << " 已连接，开始发送消息..." << std::endl;
        
        // 发送消息
        for (int i = 0; i < messageCount && running; i++) {
            std::stringstream ss;
            ss << "[Client-" << clientId << "][MSG-" << i << "] "
               << "Performance test message with some payload data to simulate real log content. "
               << "Random value: " << dis(gen);
            
            std::string message = ss.str();
            DWORD bytesWritten;
            
            BOOL success = WriteFile(
                hPipe,
                message.c_str(),
                (DWORD)message.length(),
                &bytesWritten,
                NULL
            );
            
            if (success) {
                // 等待确认
                char ackBuffer[10];
                DWORD bytesRead;
                if (ReadFile(hPipe, ackBuffer, sizeof(ackBuffer), &bytesRead, NULL)) {
                    totalSent++;
                } else {
                    totalFailed++;
                }
            } else {
                totalFailed++;
                std::cerr << "客户端 " << clientId << " 发送失败" << std::endl;
                break;
            }
            
            // 随机延迟
            if (delayMs > 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(dis(gen)));
            }
        }
        
        CloseHandle(hPipe);
        std::cout << "客户端 " << clientId << " 完成发送" << std::endl;
    }
    
public:
    void runTest(int numClients, int messagesPerClient, int maxDelayMs) {
        std::cout << "===== 性能测试配置 =====" << std::endl;
        std::cout << "客户端数量: " << numClients << std::endl;
        std::cout << "每客户端消息数: " << messagesPerClient << std::endl;
        std::cout << "总消息数: " << (numClients * messagesPerClient) << std::endl;
        std::cout << "最大延迟: " << maxDelayMs << " ms" << std::endl;
        std::cout << "========================\n" << std::endl;
        
        auto startTime = std::chrono::high_resolution_clock::now();
        
        // 启动所有客户端线程
        std::vector<std::thread> threads;
        for (int i = 0; i < numClients; i++) {
            threads.emplace_back(&PerformanceTestClient::clientWorker, this, 
                               i + 1, messagesPerClient, maxDelayMs);
            
            // 稍微错开启动时间，避免同时连接
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        
        // 等待所有线程完成
        for (auto& t : threads) {
            t.join();
        }
        
        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
        
        // 输出测试结果
        std::cout << "\n===== 测试结果 =====" << std::endl;
        std::cout << "总耗时: " << duration.count() << " ms" << std::endl;
        std::cout << "成功发送: " << totalSent.load() << " 条" << std::endl;
        std::cout << "发送失败: " << totalFailed.load() << " 条" << std::endl;
        
        if (duration.count() > 0) {
            double throughput = (double)totalSent.load() * 1000.0 / duration.count();
            std::cout << "吞吐量: " << throughput << " msg/s" << std::endl;
        }
        
        double successRate = totalSent.load() > 0 ? 
            (double)totalSent.load() / (totalSent.load() + totalFailed.load()) * 100.0 : 0;
        std::cout << "成功率: " << successRate << "%" << std::endl;
        std::cout << "===================" << std::endl;
    }
    
    void stop() {
        running = false;
    }
};

int main() {
    SetConsoleOutputCP(CP_UTF8);
    
    std::cout << "Windows日志收集系统 - 性能测试工具\n" << std::endl;
    
    PerformanceTestClient tester;
    
    // 设置 Ctrl+C 处理
    SetConsoleCtrlHandler([](DWORD signal) -> BOOL {
        if (signal == CTRL_C_EVENT) {
            std::cout << "\n收到退出信号..." << std::endl;
            return TRUE;
        }
        return FALSE;
    }, TRUE);
    
    while (true) {
        std::cout << "\n选择测试模式：" << std::endl;
        std::cout << "1. 轻量级测试 (10客户端, 100消息/客户端)" << std::endl;
        std::cout << "2. 中等负载测试 (50客户端, 200消息/客户端)" << std::endl;
        std::cout << "3. 高负载测试 (100客户端, 500消息/客户端)" << std::endl;
        std::cout << "4. 压力测试 (200客户端, 1000消息/客户端)" << std::endl;
        std::cout << "5. 自定义测试" << std::endl;
        std::cout << "0. 退出" << std::endl;
        
        int choice;
        std::cout << "请选择: ";
        std::cin >> choice;
        
        if (choice == 0) break;
        
        int numClients = 0;
        int messagesPerClient = 0;
        int maxDelay = 0;
        
        switch (choice) {
            case 1:
                numClients = 10;
                messagesPerClient = 100;
                maxDelay = 10;
                break;
            case 2:
                numClients = 50;
                messagesPerClient = 200;
                maxDelay = 5;
                break;
            case 3:
                numClients = 100;
                messagesPerClient = 500;
                maxDelay = 2;
                break;
            case 4:
                numClients = 200;
                messagesPerClient = 1000;
                maxDelay = 0;
                break;
            case 5:
                std::cout << "输入客户端数量: ";
                std::cin >> numClients;
                std::cout << "输入每客户端消息数: ";
                std::cin >> messagesPerClient;
                std::cout << "输入最大延迟(ms): ";
                std::cin >> maxDelay;
                break;
            default:
                std::cout << "无效选择" << std::endl;
                continue;
        }
        
        if (numClients > 0 && messagesPerClient > 0) {
            std::cout << "\n开始测试，请确保日志服务器已启动..." << std::endl;
            std::cout << "按任意键开始...";
            std::cin.ignore();
            std::cin.get();
            
            tester.runTest(numClients, messagesPerClient, maxDelay);
        }
    }
    
    return 0;
}