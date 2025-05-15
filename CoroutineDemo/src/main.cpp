#include "CoroutineTask.h"
#include <iostream>
#include <thread>
#include <vector>
#include <chrono>
#include <boost/asio/thread_pool.hpp>
// 在 main.cpp 顶部，或者某个单例里
static std::vector<Task<void>> g_tasks;
// 全局线程池
static boost::asio::thread_pool globalPool{ std::max(1u, std::thread::hardware_concurrency()) };

std::vector<int> RunAStar(int s, int e) {
    std::this_thread::sleep_for(std::chrono::milliseconds(50));  // 模拟耗时
    return { s, s + 1, e };
}


// 协程异步函数
Task<void> ComputePathAsync(int entityId, int s, int e) {
    co_await ThreadPoolAwaiter{ globalPool };
    auto path = RunAStar(s, e);
    co_await MainThreadAwaiter{};
    std::cout << "Entity " << entityId << " path size=" << path.size() << "\n";
}
// 不用 Launch，直接把 Task 保存在容器里
void OnRequestPath(int eid, int s, int e) {
    // 1) 创建协程，立即跑到第一个 co_await(ThreadPoolAwaiter) 并挂起
    auto task = ComputePathAsync(eid, s, e);

    // 2) 把它放入全局容器，保证析构的时候还能活着
    g_tasks.emplace_back(std::move(task));
}



int main() {
// 模拟一次请求
    OnRequestPath(42, 1, 99);

    for (int frame = 0; frame < 10; ++frame) {
        std::cout << "Frame " << frame << "\n";
        MainThreadDispatcher::instance().run_pending();
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
    return 0;
}
