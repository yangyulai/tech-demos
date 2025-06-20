// SharedMemoryLogServer.cpp - 基于共享内存的日志收集服务器
#include <windows.h>
#include <iostream>
#include <thread>
#include <atomic>
#include <chrono>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <iomanip>
#include <cstring>

// 共享内存结构定义
#pragma pack(push, 1)
struct SharedMemoryHeader {
    static constexpr size_t MAGIC = 0x4C4F474D;  // "LOGM"
    static constexpr int VERSION = 1;
    
    size_t magic;                    // 魔数，用于验证
    int version;                     // 版本号
    std::atomic<uint64_t> writeIndex; // 环形缓冲区写入位置
    std::atomic<uint64_t> readIndex;  // 环形缓冲区读取位置
    std::atomic<uint32_t> activeWriters; // 活跃写入者数量
    size_t bufferSize;               // 缓冲区大小
    size_t slotSize;                 // 每个槽位大小
    size_t slotCount;                // 槽位数量
    
    // 统计信息
    std::atomic<uint64_t> totalMessages;
    std::atomic<uint64_t> droppedMessages;
    std::atomic<uint64_t> totalBytes;
};

struct LogSlot {
    static constexpr int SLOT_EMPTY = 0;
    static constexpr int SLOT_WRITING = 1;
    static constexpr int SLOT_READY = 2;
    static constexpr int SLOT_READING = 3;
    
    std::atomic<int> status;         // 槽位状态
    DWORD processId;                 // 写入进程ID
    DWORD threadId;                  // 写入线程ID
    SYSTEMTIME timestamp;            // 时间戳
    uint32_t dataLength;             // 实际数据长度
    char data[1];                    // 柔性数组，实际大小由slotSize决定
};
#pragma pack(pop)

class SharedMemoryLogServer {
private:
    static constexpr size_t DEFAULT_BUFFER_SIZE = 100 * 1024 * 1024;  // 100MB
    static constexpr size_t DEFAULT_SLOT_SIZE = 4096;                 // 4KB per slot
    static constexpr LPCWSTR SHARED_MEMORY_NAME = L"LogCollectorSharedMemory";
    static constexpr LPCWSTR MUTEX_NAME = L"LogCollectorMutex";
    static constexpr LPCWSTR EVENT_NAME = L"LogCollectorEvent";
    
    HANDLE hMapFile;
    HANDLE hMutex;
    HANDLE hEvent;
    void* pBuffer;
    SharedMemoryHeader* pHeader;
    
    std::atomic<bool> running;
    std::thread readerThread;
    std::thread statsThread;
    std::ofstream logFile;
    
    // 统计信息
    std::atomic<uint64_t> messagesProcessed{0};
    std::atomic<uint64_t> bytesProcessed{0};
    
    // 获取槽位指针
    LogSlot* getSlot(size_t index) const
    {
        char* base = reinterpret_cast<char*>(pBuffer) + sizeof(SharedMemoryHeader);
        return reinterpret_cast<LogSlot*>(base + (index * pHeader->slotSize));
    }
    
    // 读取线程函数
    void readerThreadFunc() {
        std::vector<char> batch;
        batch.reserve(1024 * 1024);  // 1MB批量缓冲
        
        while (running) {
            uint64_t readIdx = pHeader->readIndex.load(std::memory_order_acquire);
            uint64_t writeIdx = pHeader->writeIndex.load(std::memory_order_acquire);
            
            // 检查是否有新数据
            if (readIdx == writeIdx) {
                // 等待新数据
                WaitForSingleObject(hEvent, 100);
                continue;
            }
            
            // 批量读取
            size_t messagesInBatch = 0;
            while (readIdx != writeIdx && messagesInBatch < 100) {
                size_t slotIndex = readIdx % pHeader->slotCount;
                LogSlot* slot = getSlot(slotIndex);
                
                // 等待槽位准备好
                int expected = LogSlot::SLOT_READY;
                if (!slot->status.compare_exchange_strong(expected, LogSlot::SLOT_READING)) {
                    // 槽位还未准备好，等待
                    std::this_thread::yield();
                    continue;
                }
                
                // 读取数据
                if (slot->dataLength > 0 && slot->dataLength < pHeader->slotSize) {
                    // 格式化日志
                    std::stringstream ss;
                    ss << "[" << formatTimestamp(slot->timestamp) << "]"
                        << "[PID:" << slot->processId << "]"
                        << "[TID:" << slot->threadId << "] ";

                    std::string prefix = ss.str();
                    batch.insert(batch.end(), prefix.begin(), prefix.end());
                    batch.insert(batch.end(), slot->data, slot->data + slot->dataLength);
                    batch.push_back('\n');

                    ++messagesProcessed;
                    bytesProcessed += slot->dataLength;
                    messagesInBatch++;
                }
                
                // 标记槽位为空
                slot->status.store(LogSlot::SLOT_EMPTY, std::memory_order_release);
                
                // 更新读取索引
                readIdx++;
                pHeader->readIndex.store(readIdx, std::memory_order_release);
            }
            
            // 批量写入文件
            if (!batch.empty()) {
                logFile.write(batch.data(), batch.size());
                logFile.flush();
                
                // 输出部分到控制台（可选）
                if (messagesInBatch <= 5) {
                    std::cout.write(batch.data(), batch.size());
                }
                
                batch.clear();
            }
        }
    }
    
    // 统计线程函数
    void statsThreadFunc() {
        auto lastTime = std::chrono::steady_clock::now();
        uint64_t lastProcessed = 0;
        uint64_t lastBytes = 0;
        
        while (running) {
            std::this_thread::sleep_for(std::chrono::seconds(5));
            
            auto currentTime = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(currentTime - lastTime).count();
            
            if (elapsed > 0) {
                uint64_t currentProcessed = messagesProcessed.load();
                uint64_t currentBytes = bytesProcessed.load();
                
                uint64_t readIdx = pHeader->readIndex.load();
                uint64_t writeIdx = pHeader->writeIndex.load();
                uint64_t pending = (writeIdx >= readIdx) ? (writeIdx - readIdx) : 0;
                
                uint64_t messageDelta = currentProcessed - lastProcessed;
                uint64_t bytesDelta = currentBytes - lastBytes;
                
                std::cout << "\n===== 共享内存日志服务器统计 =====" << '\n';
                std::cout << "活跃写入者: " << pHeader->activeWriters.load() << '\n';
                std::cout << "待处理消息: " << pending << "/" << pHeader->slotCount << '\n';
                std::cout << "处理速率: " << (messageDelta / elapsed) << " msg/s" << '\n';
                std::cout << "吞吐量: " << formatBytes(bytesDelta / elapsed) << "/s" << '\n';
                std::cout << "总处理消息: " << currentProcessed << '\n';
                std::cout << "总处理字节: " << formatBytes(currentBytes) << '\n';
                std::cout << "总接收消息: " << pHeader->totalMessages.load() << '\n';
                std::cout << "丢弃消息: " << pHeader->droppedMessages.load() << '\n';
                std::cout << "=================================\n" << '\n';
                
                lastTime = currentTime;
                lastProcessed = currentProcessed;
                lastBytes = currentBytes;
            }
        }
    }
    
    // 格式化时间戳
    std::string formatTimestamp(const SYSTEMTIME& st) {
        std::stringstream ss;
        ss << std::setfill('0') 
           << std::setw(4) << st.wYear << "-"
           << std::setw(2) << st.wMonth << "-"
           << std::setw(2) << st.wDay << " "
           << std::setw(2) << st.wHour << ":"
           << std::setw(2) << st.wMinute << ":"
           << std::setw(2) << st.wSecond << "."
           << std::setw(3) << st.wMilliseconds;
        return ss.str();
    }
    
    // 格式化字节数
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
    
public:
    SharedMemoryLogServer() : hMapFile(NULL), hMutex(NULL), hEvent(NULL), 
                             pBuffer(nullptr), pHeader(nullptr), running(false) {
    }
    
    ~SharedMemoryLogServer() {
        stop();
        cleanup();
    }
    
    bool initialize(size_t bufferSize = DEFAULT_BUFFER_SIZE, 
                   size_t slotSize = DEFAULT_SLOT_SIZE) {
        // 创建或打开互斥体
        hMutex = CreateMutex(NULL, FALSE, MUTEX_NAME);
        if (!hMutex) {
            std::cerr << "创建互斥体失败: " << GetLastError() << '\n';
            return false;
        }
        
        // 创建或打开事件
        hEvent = CreateEvent(NULL, FALSE, FALSE, EVENT_NAME);
        if (!hEvent) {
            std::cerr << "创建事件失败: " << GetLastError() << '\n';
            return false;
        }
        
        // 计算实际需要的内存大小
        size_t slotCount = (bufferSize - sizeof(SharedMemoryHeader)) / slotSize;
        size_t totalSize = sizeof(SharedMemoryHeader) + (slotCount * slotSize);
        
        // 创建共享内存
        hMapFile = CreateFileMapping(
            INVALID_HANDLE_VALUE,
            NULL,
            PAGE_READWRITE,
            (DWORD)(totalSize >> 32),
            (DWORD)(totalSize & 0xFFFFFFFF),
            SHARED_MEMORY_NAME
        );
        
        if (!hMapFile) {
            std::cerr << "创建共享内存失败: " << GetLastError() << '\n';
            return false;
        }
        
        bool isNewMemory = (GetLastError() != ERROR_ALREADY_EXISTS);
        
        // 映射共享内存
        pBuffer = MapViewOfFile(
            hMapFile,
            FILE_MAP_ALL_ACCESS,
            0, 0, 0
        );
        
        if (!pBuffer) {
            std::cerr << "映射共享内存失败: " << GetLastError() << '\n';
            return false;
        }
        
        pHeader = reinterpret_cast<SharedMemoryHeader*>(pBuffer);
        
        // 初始化共享内存头（仅在新创建时）
        if (isNewMemory) {
            pHeader->magic = SharedMemoryHeader::MAGIC;
            pHeader->version = SharedMemoryHeader::VERSION;
            pHeader->writeIndex.store(0);
            pHeader->readIndex.store(0);
            pHeader->activeWriters.store(0);
            pHeader->bufferSize = totalSize;
            pHeader->slotSize = slotSize;
            pHeader->slotCount = slotCount;
            pHeader->totalMessages.store(0);
            pHeader->droppedMessages.store(0);
            pHeader->totalBytes.store(0);
            
            // 初始化所有槽位
            for (size_t i = 0; i < slotCount; i++) {
                LogSlot* slot = getSlot(i);
                slot->status.store(LogSlot::SLOT_EMPTY);
            }
            
            std::cout << "创建新的共享内存，大小: " << formatBytes(totalSize) 
                     << "，槽位数: " << slotCount << '\n';
        } else {
            // 验证现有共享内存
            if (pHeader->magic != SharedMemoryHeader::MAGIC) {
                std::cerr << "共享内存格式无效" << '\n';
                return false;
            }
            
            std::cout << "连接到现有共享内存，大小: " << formatBytes(pHeader->bufferSize) 
                     << "，槽位数: " << pHeader->slotCount << '\n';
        }
        
        // 打开日志文件
        logFile.open("shared_memory_logs.txt", std::ios::app);
        if (!logFile.is_open()) {
            std::cerr << "无法打开日志文件" << '\n';
            return false;
        }
        
        return true;
    }
    
    void start() {
        running = true;
        
        // 启动读取线程
        readerThread = std::thread(&SharedMemoryLogServer::readerThreadFunc, this);
        
        // 启动统计线程
        statsThread = std::thread(&SharedMemoryLogServer::statsThreadFunc, this);
        
        std::cout << "共享内存日志服务器已启动" << '\n';
        std::cout << "共享内存名称: " << "LogCollectorSharedMemory" << '\n';
        std::cout << "按 Ctrl+C 退出\n" << '\n';
        
        // 主线程等待
        while (running) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
    
    void stop() {
        running = false;
        
        if (hEvent) {
            SetEvent(hEvent);  // 唤醒读取线程
        }
        
        if (readerThread.joinable()) {
            readerThread.join();
        }
        
        if (statsThread.joinable()) {
            statsThread.join();
        }
        
        if (logFile.is_open()) {
            logFile.close();
        }
    }
    
    void cleanup() {
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
};

// Ctrl+C 处理器
BOOL WINAPI ConsoleHandler(DWORD signal) {
    if (signal == CTRL_C_EVENT) {
        std::cout << "\n收到退出信号..." << '\n';
        return TRUE;
    }
    return FALSE;
}

int main(int argc, char* argv[]) {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCtrlHandler(ConsoleHandler, TRUE);
    
    // 解析命令行参数
    size_t bufferSize = 100 * 1024 * 1024;  // 默认100MB
    size_t slotSize = 4096;                 // 默认4KB
    
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--buffer-size" && i + 1 < argc) {
            bufferSize = std::stoull(argv[++i]) * 1024 * 1024;  // MB to bytes
        } else if (arg == "--slot-size" && i + 1 < argc) {
            slotSize = std::stoull(argv[++i]) * 1024;  // KB to bytes
        } else if (arg == "--help") {
            std::cout << "用法: SharedMemoryLogServer [选项]" << '\n';
            std::cout << "选项:" << '\n';
            std::cout << "  --buffer-size <MB>  设置共享内存大小（默认: 100MB）" << '\n';
            std::cout << "  --slot-size <KB>    设置槽位大小（默认: 4KB）" << '\n';
            std::cout << "  --help              显示帮助信息" << '\n';
            return 0;
        }
    }
    
    try {
        SharedMemoryLogServer server;
        
        if (!server.initialize(bufferSize, slotSize)) {
            std::cerr << "服务器初始化失败" << '\n';
            return 1;
        }
        
        server.start();
        
    } catch (const std::exception& e) {
        std::cerr << "错误: " << e.what() << '\n';
        return 1;
    }
    
    return 0;
}