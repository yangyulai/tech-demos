// SharedMemoryLogClient.cpp - 基于共享内存的日志客户端
#include <windows.h>
#include <iostream>
#include <string>
#include <sstream>
#include <atomic>
#include <chrono>
#include <iomanip>
#include <thread>
#include <vector>

// 复用服务器的结构定义
#pragma pack(push, 1)
struct SharedMemoryHeader {
    static constexpr size_t MAGIC = 0x4C4F474D;  // "LOGM"
    static constexpr int VERSION = 1;
    
    size_t magic;
    int version;
    std::atomic<uint64_t> writeIndex;
    std::atomic<uint64_t> readIndex;
    std::atomic<uint32_t> activeWriters;
    size_t bufferSize;
    size_t slotSize;
    size_t slotCount;
    
    std::atomic<uint64_t> totalMessages;
    std::atomic<uint64_t> droppedMessages;
    std::atomic<uint64_t> totalBytes;
};

struct LogSlot {
    static constexpr int SLOT_EMPTY = 0;
    static constexpr int SLOT_WRITING = 1;
    static constexpr int SLOT_READY = 2;
    static constexpr int SLOT_READING = 3;
    
    std::atomic<int> status;
    DWORD processId;
    DWORD threadId;
    SYSTEMTIME timestamp;
    uint32_t dataLength;
    char data[1];
};
#pragma pack(pop)

class SharedMemoryLogClient {
private:
    static constexpr LPCWSTR SHARED_MEMORY_NAME = L"LogCollectorSharedMemory";
    static constexpr LPCWSTR MUTEX_NAME = L"LogCollectorMutex";
    static constexpr LPCWSTR EVENT_NAME = L"LogCollectorEvent";
    
    HANDLE hMapFile;
    HANDLE hMutex;
    HANDLE hEvent;
    void* pBuffer;
    SharedMemoryHeader* pHeader;
    std::string clientName;
    
    // 统计信息
    std::atomic<uint64_t> messagesSent{0};
    std::atomic<uint64_t> messagesDropped{0};
    
    // 获取槽位指针
    LogSlot* getSlot(size_t index) {
        char* base = reinterpret_cast<char*>(pBuffer) + sizeof(SharedMemoryHeader);
        return reinterpret_cast<LogSlot*>(base + (index * pHeader->slotSize));
    }
    
public:
    SharedMemoryLogClient(const std::string& name = "") 
        : hMapFile(NULL), hMutex(NULL), hEvent(NULL), 
          pBuffer(nullptr), pHeader(nullptr), clientName(name) {
        if (clientName.empty()) {
            clientName = "Client_" + std::to_string(GetCurrentProcessId());
        }
    }
    
    ~SharedMemoryLogClient() {
        disconnect();
    }
    
    bool connect() {
        // 打开互斥体
        hMutex = OpenMutex(MUTEX_ALL_ACCESS, FALSE, MUTEX_NAME);
        if (!hMutex) {
            std::cerr << "无法打开互斥体: " << GetLastError() << std::endl;
            return false;
        }
        
        // 打开事件
        hEvent = OpenEvent(EVENT_ALL_ACCESS, FALSE, EVENT_NAME);
        if (!hEvent) {
            std::cerr << "无法打开事件: " << GetLastError() << std::endl;
            return false;
        }
        
        // 打开共享内存
        hMapFile = OpenFileMapping(
            FILE_MAP_ALL_ACCESS,
            FALSE,
            SHARED_MEMORY_NAME
        );
        
        if (!hMapFile) {
            std::cerr << "无法打开共享内存: " << GetLastError() << std::endl;
            return false;
        }
        
        // 映射共享内存
        pBuffer = MapViewOfFile(
            hMapFile,
            FILE_MAP_ALL_ACCESS,
            0, 0, 0
        );
        
        if (!pBuffer) {
            std::cerr << "无法映射共享内存: " << GetLastError() << std::endl;
            return false;
        }
        
        pHeader = reinterpret_cast<SharedMemoryHeader*>(pBuffer);
        
        // 验证共享内存
        if (pHeader->magic != SharedMemoryHeader::MAGIC) {
            std::cerr << "共享内存格式无效" << std::endl;
            return false;
        }
        
        // 增加活跃写入者计数
        pHeader->activeWriters.fetch_add(1);
        
        std::cout << "已连接到共享内存日志服务器" << std::endl;
        std::cout << "缓冲区大小: " << formatBytes(pHeader->bufferSize) 
                  << ", 槽位数: " << pHeader->slotCount << std::endl;
        
        return true;
    }
    
    void disconnect() {
        if (pHeader) {
            pHeader->activeWriters.fetch_sub(1);
        }
        
        if (pBuffer) {
            UnmapViewOfFile(pBuffer);
            pBuffer = nullptr;
        }
        
        if (hMapFile) {
            CloseHandle(hMapFile);
            hMapFile = NULL;
        }
        
        if (hEvent) {
            CloseHandle(hEvent);
            hEvent = NULL;
        }
        
        if (hMutex) {
            CloseHandle(hMutex);
            hMutex = NULL;
        }
    }
    
    bool sendLog(const std::string& level, const std::string& message) {
        if (!pHeader) {
            return false;
        }
        
        // 构建完整消息
        std::stringstream ss;
        ss << "[" << clientName << "][" << level << "] " << message;
        std::string fullMessage = ss.str();
        
        // 检查消息大小
        if (fullMessage.length() >= pHeader->slotSize - offsetof(LogSlot, data)) {
            std::cerr << "消息过大，最大支持: " 
                     << (pHeader->slotSize - offsetof(LogSlot, data)) << " 字节" << std::endl;
            return false;
        }
        
        // 获取写入位置
        uint64_t writeIdx = pHeader->writeIndex.fetch_add(1);
        size_t slotIndex = writeIdx % pHeader->slotCount;
        LogSlot* slot = getSlot(slotIndex);
        
        // 等待槽位可用（带超时）
        auto startTime = std::chrono::steady_clock::now();
        int expected = LogSlot::SLOT_EMPTY;
        
        while (!slot->status.compare_exchange_weak(expected, LogSlot::SLOT_WRITING)) {
            // 检查超时（100ms）
            auto elapsed = std::chrono::steady_clock::now() - startTime;
            if (elapsed > std::chrono::milliseconds(100)) {
                messagesDropped++;
                pHeader->droppedMessages.fetch_add(1);
                return false;
            }
            
            expected = LogSlot::SLOT_EMPTY;
            std::this_thread::yield();
        }
        
        // 写入数据
        slot->processId = GetCurrentProcessId();
        slot->threadId = GetCurrentThreadId();
        GetLocalTime(&slot->timestamp);
        slot->dataLength = static_cast<uint32_t>(fullMessage.length());
        memcpy(slot->data, fullMessage.c_str(), fullMessage.length());
        
        // 更新统计
        pHeader->totalMessages.fetch_add(1);
        pHeader->totalBytes.fetch_add(fullMessage.length());
        messagesSent++;
        
        // 标记槽位准备就绪
        slot->status.store(LogSlot::SLOT_READY, std::memory_order_release);
        
        // 通知服务器
        SetEvent(hEvent);
        
        return true;
    }
    
    // 便捷方法
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
    
    // 批量发送（性能优化）
    void sendBatch(const std::vector<std::string>& messages) {
        for (const auto& msg : messages) {
            sendLog("INFO", msg);
        }
    }
    
    // 获取统计信息
    void printStats() {
        std::cout << "\n客户端统计信息:" << std::endl;
        std::cout << "已发送: " << messagesSent.load() << " 条" << std::endl;
        std::cout << "已丢弃: " << messagesDropped.load() << " 条" << std::endl;
        if (messagesSent > 0) {
            double dropRate = (double)messagesDropped / (messagesSent + messagesDropped) * 100.0;
            std::cout << "丢弃率: " << dropRate << "%" << std::endl;
        }
    }
    
private:
    std::string formatBytes(uint64_t bytes) {
        const char* units[] = {"B", "KB", "MB", "GB"};
        int unitIndex = 0;
        double size = static_cast<double>(bytes);
        
        while (size >= 1024.0 && unitIndex < 3) {
            size /= 1024.0;
            unitIndex++;
        }
        
        std::stringstream ss;
        ss << std::fixed << std::setprecision(2) << size << " " << units[unitIndex];
        return ss.str();
    }
};

// 性能测试函数
void performanceTest(int messageCount, int delayUs) {
    SharedMemoryLogClient client("PerfTest");
    
    if (!client.connect()) {
        std::cerr << "连接失败" << std::endl;
        return;
    }
    
    std::cout << "开始性能测试: " << messageCount << " 条消息, 延迟: " << delayUs << " 微秒" << std::endl;
    
    auto startTime = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < messageCount; i++) {
        std::string msg = "Performance test message #" + std::to_string(i) + 
                         " with some payload data to simulate real log content.";
        client.logInfo(msg);
        
        if (delayUs > 0) {
            std::this_thread::sleep_for(std::chrono::microseconds(delayUs));
        }
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    
    std::cout << "\n性能测试完成:" << std::endl;
    std::cout << "耗时: " << duration.count() << " ms" << std::endl;
    
    if (duration.count() > 0) {
        double throughput = (double)messageCount * 1000.0 / duration.count();
        std::cout << "吞吐量: " << throughput << " msg/s" << std::endl;
    }
    
    client.printStats();
}

// 多线程压力测试
void stressTest(int threadCount, int messagesPerThread) {
    std::vector<std::thread> threads;
    auto startTime = std::chrono::high_resolution_clock::now();
    
    std::cout << "启动压力测试: " << threadCount << " 线程, 每线程 " 
              << messagesPerThread << " 条消息" << std::endl;
    
    for (int t = 0; t < threadCount; t++) {
        threads.emplace_back([t, messagesPerThread]() {
            SharedMemoryLogClient client("Thread_" + std::to_string(t));
            
            if (!client.connect()) {
                std::cerr << "线程 " << t << " 连接失败" << std::endl;
                return;
            }
            
            for (int i = 0; i < messagesPerThread; i++) {
                std::string msg = "Thread " + std::to_string(t) + 
                                 " message " + std::to_string(i);
                
                switch (i % 4) {
                    case 0: client.logInfo(msg); break;
                    case 1: client.logDebug(msg); break;
                    case 2: client.logWarning(msg); break;
                    case 3: client.logError(msg); break;
                }
            }
            
            client.printStats();
        });
    }
    
    // 等待所有线程完成
    for (auto& t : threads) {
        t.join();
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    
    std::cout << "\n压力测试完成:" << std::endl;
    std::cout << "总耗时: " << duration.count() << " ms" << std::endl;
    std::cout << "总消息数: " << (threadCount * messagesPerThread) << std::endl;
    
    if (duration.count() > 0) {
        double throughput = (double)(threadCount * messagesPerThread) * 1000.0 / duration.count();
        std::cout << "总吞吐量: " << throughput << " msg/s" << std::endl;
    }
}

int main() {
    SetConsoleOutputCP(CP_UTF8);
    
    std::cout << "共享内存日志客户端\n" << std::endl;
    
    while (true) {
        std::cout << "\n选择模式:" << std::endl;
        std::cout << "1. 交互式日志发送" << std::endl;
        std::cout << "2. 性能测试" << std::endl;
        std::cout << "3. 多线程压力测试" << std::endl;
        std::cout << "4. 批量发送测试" << std::endl;
        std::cout << "0. 退出" << std::endl;
        
        int choice;
        std::cout << "请选择: ";
        std::cin >> choice;
        std::cin.ignore();
        
        if (choice == 0) break;
        
        switch (choice) {
            case 1: {
                SharedMemoryLogClient client("Interactive");
                
                if (!client.connect()) {
                    std::cerr << "连接失败" << std::endl;
                    break;
                }
                
                std::cout << "已连接，输入日志消息（输入 'quit' 退出）:" << std::endl;
                
                std::string line;
                while (std::getline(std::cin, line)) {
                    if (line == "quit") break;
                    
                    if (line.empty()) continue;
                    
                    // 解析日志级别
                    if (line.substr(0, 6) == "[INFO]") {
                        client.logInfo(line.substr(6));
                    } else if (line.substr(0, 6) == "[WARN]") {
                        client.logWarning(line.substr(6));
                    } else if (line.substr(0, 7) == "[ERROR]") {
                        client.logError(line.substr(7));
                    } else if (line.substr(0, 7) == "[DEBUG]") {
                        client.logDebug(line.substr(7));
                    } else {
                        client.logInfo(line);
                    }
                }
                
                client.printStats();
                break;
            }
            
            case 2: {
                int count, delay;
                std::cout << "输入消息数量: ";
                std::cin >> count;
                std::cout << "输入延迟（微秒，0表示无延迟）: ";
                std::cin >> delay;
                
                performanceTest(count, delay);
                break;
            }
            
            case 3: {
                int threads, messages;
                std::cout << "输入线程数: ";
                std::cin >> threads;
                std::cout << "输入每线程消息数: ";
                std::cin >> messages;
                
                stressTest(threads, messages);
                break;
            }
            
            case 4: {
                SharedMemoryLogClient client("BatchTest");
                
                if (!client.connect()) {
                    std::cerr << "连接失败" << std::endl;
                    break;
                }
                
                std::cout << "准备批量发送1000条消息..." << std::endl;
                
                std::vector<std::string> batch;
                for (int i = 0; i < 1000; i++) {
                    batch.push_back("Batch message #" + std::to_string(i));
                }
                
                auto startTime = std::chrono::high_resolution_clock::now();
                client.sendBatch(batch);
                auto endTime = std::chrono::high_resolution_clock::now();
                
                auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
                std::cout << "批量发送完成，耗时: " << duration.count() << " ms" << std::endl;
                
                client.printStats();
                break;
            }
        }
    }
    
    return 0;
}