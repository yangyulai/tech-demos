// LogCollectorServer.cpp - 日志收集服务端
#include <windows.h>
#include <iostream>
#include <vector>
#include <thread>
#include <string>
#include <mutex>
#include <fstream>
#include <sstream>
#include <chrono>
#include <iomanip>
#include <queue>

class NamedPipeServer {
public:
	NamedPipeServer(const NamedPipeServer& other) = delete;
	NamedPipeServer(NamedPipeServer&& other) noexcept = delete;
	NamedPipeServer& operator=(const NamedPipeServer& other) = delete;
	NamedPipeServer& operator=(NamedPipeServer&& other) noexcept = delete;

private:
    static constexpr int MAX_INSTANCES = 100;  // 最大并发连接数
    static constexpr int BUFFER_SIZE = 4096;   // 缓冲区大小
    static constexpr LPCWSTR PIPE_NAME = L"\\\\.\\pipe\\LogCollectorPipe";
    static constexpr size_t MAX_QUEUE_SIZE = 10000; // 最大队列大小

    // 日志消息结构
    struct LogMessage {
        std::string timestamp;
        std::string content;

        LogMessage(const std::string& ts, const std::string& msg)
            : timestamp(ts), content(msg) {
        }
    };
    // 消息队列相关
    std::queue<LogMessage> messageQueue;
    std::mutex queueMutex;
    std::condition_variable queueCV;
    std::thread writerThread;

    // 统计信息
    std::atomic<uint64_t> totalReceived{ 0 };
    std::atomic<uint64_t> totalWritten{ 0 };
    std::atomic<uint64_t> queueOverflow{ 0 };
    std::atomic<uint32_t> activeConnections{ 0 };

    std::mutex logMutex;
    std::ofstream logFile;
    bool running;
    std::vector<std::thread> threads;
    std::thread statsThread;

    // 获取当前时间戳
	static std::string getCurrentTimestamp() {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        std::stringstream ss;
        ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
        return ss.str();
    }
    // 日志写入线程
    void writerThreadFunc() {
        std::vector<LogMessage> batch;
        batch.reserve(100); // 批量写入优化

        while (running || !messageQueue.empty()) {
            {
                std::unique_lock<std::mutex> lock(queueMutex);

                // 等待消息或退出信号
                queueCV.wait(lock,[this] {
                    return !messageQueue.empty() || !running;
                    });

                // 批量获取消息
                while (!messageQueue.empty() && batch.size() < 100) {
                    batch.push_back(std::move(messageQueue.front()));
                    messageQueue.pop();
                }
            }

            // 批量写入文件
            if (!batch.empty()) {
                for (const auto& msg : batch) {
                    logFile << "[" << msg.timestamp << "] " << msg.content << '\n';
                    ++totalWritten;
                }
                logFile.flush();

                batch.clear();
            }
        }

        std::cout << "日志写入线程已退出" << '\n';
    }
    // 统计信息线程
    void statsThreadFunc() {
        auto lastTime = std::chrono::steady_clock::now();
        uint64_t lastReceived = 0;
        uint64_t lastWritten = 0;

        while (running) {
            std::this_thread::sleep_for(std::chrono::seconds(5));

            auto currentTime = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(currentTime - lastTime).count();

            uint64_t currentReceived = totalReceived.load();
            uint64_t currentWritten = totalWritten.load();

            uint64_t receivedDelta = currentReceived - lastReceived;
            uint64_t writtenDelta = currentWritten - lastWritten;

            if (elapsed > 0) {
                size_t queueSize = 0;
                {
                    std::lock_guard<std::mutex> lock(queueMutex);
                    queueSize = messageQueue.size();
                }

                std::cout << "\n===== 统计信息 =====" << '\n';
                std::cout << "活跃连接数: " << activeConnections.load() << '\n';
                std::cout << "队列大小: " << queueSize << "/" << MAX_QUEUE_SIZE << '\n';
                std::cout << "接收速率: " << (receivedDelta / elapsed) << " msg/s" << '\n';
                std::cout << "写入速率: " << (writtenDelta / elapsed) << " msg/s" << '\n';
                std::cout << "总接收: " << currentReceived << '\n';
                std::cout << "总写入: " << currentWritten << '\n';
                std::cout << "队列溢出: " << queueOverflow.load() << '\n';
                std::cout << "==================\n" << '\n';
            }

            lastTime = currentTime;
            lastReceived = currentReceived;
            lastWritten = currentWritten;
        }
    }
    // 处理单个客户端连接
    void handleClient(HANDLE hPipe) {
        // 1. 取 PID  
        DWORD clientPid = 0;
        if (GetNamedPipeClientProcessId(hPipe, &clientPid)) {
            std::cout << "[" << getCurrentTimestamp() << "] "
                << "客户端 PID=" << clientPid << " 已连接\n";
        }

        char buffer[BUFFER_SIZE];
        DWORD bytesRead;

        while (running) {
            // 读取客户端消息
            BOOL success = ReadFile(
                hPipe,
                buffer,
                BUFFER_SIZE - 1,
                &bytesRead,
                NULL
            );
            
            if (!success || bytesRead == 0) {
                if (GetLastError() == ERROR_BROKEN_PIPE) {
                    std::cout << "客户端断开连接" << '\n';
                }
                break;
            }
            
            buffer[bytesRead] = '\0';
            
            ++totalReceived;

            // 将消息加入队列
            {
                std::lock_guard<std::mutex> lock(queueMutex);

                // 检查队列是否已满
                if (messageQueue.size() >= MAX_QUEUE_SIZE) {
                    ++queueOverflow;
                    // 可选：移除最旧的消息
                    messageQueue.pop();
                }

                messageQueue.emplace(getCurrentTimestamp(), std::string(buffer));
            }

            // 通知写入线程
            queueCV.notify_one();
            
            // 发送确认消息给客户端
            const char* ack = "ACK";
            DWORD bytesWritten;
            WriteFile(hPipe, ack, (DWORD)strlen(ack), &bytesWritten, NULL);
        }
        
        FlushFileBuffers(hPipe);
        DisconnectNamedPipe(hPipe);
        CloseHandle(hPipe);
    }
    
public:
    NamedPipeServer() : running(false) {
        // 打开日志文件
        logFile.open("collected_logs.txt", std::ios::app);
        if (!logFile.is_open()) {
            throw std::runtime_error("无法打开日志文件");
        }
    }
    
    ~NamedPipeServer() {
        stop();
        if (logFile.is_open()) {
            logFile.close();
        }
    }
    
    void start() {
        running = true;
        std::cout << "日志收集服务器启动中..." << '\n';
        // 启动日志写入线程
        writerThread = std::thread(&NamedPipeServer::writerThreadFunc, this);

        // 启动统计线程
        statsThread = std::thread(&NamedPipeServer::statsThreadFunc, this);
        while (running) {
            // 创建命名管道实例
            HANDLE hPipe = CreateNamedPipe(
                PIPE_NAME,
                PIPE_ACCESS_DUPLEX,
                PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
                MAX_INSTANCES,
                BUFFER_SIZE,
                BUFFER_SIZE,
                0,
                NULL
            );
            
            if (hPipe == INVALID_HANDLE_VALUE) {
                std::cerr << "创建命名管道失败，错误码: " << GetLastError() << '\n';
                break;
            }
            
            // 等待客户端连接
            std::cout << "等待客户端连接..." << '\n';
            BOOL connected = ConnectNamedPipe(hPipe, NULL) 
                ? TRUE 
                : (GetLastError() == ERROR_PIPE_CONNECTED);
            
            if (connected) {
                std::cout << "客户端已连接，当前活跃连接: " << (activeConnections.load() + 1) << std::endl;

                // 在新线程中处理客户端
                threads.emplace_back(&NamedPipeServer::handleClient, this, hPipe);
            } else {
                CloseHandle(hPipe);
            }
        }
        
        // 等待所有线程结束
        for (auto& t : threads) {
            if (t.joinable()) {
                t.join();
            }
        }
        std::cout << "等待日志写入完成..." << std::endl;

        // 通知写入线程退出
        queueCV.notify_all();

        // 等待写入线程和统计线程结束
        if (writerThread.joinable()) {
            writerThread.join();
        }

        if (statsThread.joinable()) {
            statsThread.join();
        }

        std::cout << "服务器已完全停止" << std::endl;
        std::cout << "最终统计 - 总接收: " << totalReceived.load()
            << ", 总写入: " << totalWritten.load()
            << ", 队列溢出: " << queueOverflow.load() << std::endl;
    }
    
    void stop() {
        running = false;
        std::cout << "正在停止日志收集服务器..." << '\n';
        queueCV.notify_all(); // 唤醒写入线程
    }
};

int main() {
    try {
        NamedPipeServer server;
        
        // 设置控制台编码为UTF-8
        SetConsoleOutputCP(CP_UTF8);
        
        std::cout << "日志收集服务器已启动，按 Ctrl+C 退出" << '\n';
        
        // 设置 Ctrl+C 处理
        SetConsoleCtrlHandler([](DWORD signal) -> BOOL {
            if (signal == CTRL_C_EVENT) {
                std::cout << "\n收到退出信号..." << '\n';
                return TRUE;
            }
            return FALSE;
        }, TRUE);
        
        server.start();
        
    } catch (const std::exception& e) {
        std::cerr << "错误: " << e.what() << '\n';
        return 1;
    }
    
    return 0;
}