#include <iostream>
#include <coroutine>
#include <thread>
#include <chrono>
#include <vector>
#include <memory>
#include <random>

// 简单的任务协程
template<typename T>
struct Task {
    struct promise_type {
        T value;
        std::exception_ptr exception;
        std::coroutine_handle<> waiting = nullptr;
        
        Task get_return_object() {
            return Task{std::coroutine_handle<promise_type>::from_promise(*this)};
        }
        
        std::suspend_never initial_suspend() { return {}; }
        
        struct final_awaiter {
            bool await_ready() noexcept { return false; }
            void await_suspend(std::coroutine_handle<promise_type> h) noexcept {
                if (h.promise().waiting) {
                    h.promise().waiting.resume();
                }
            }
            void await_resume() noexcept {}
        };
        
        final_awaiter final_suspend() noexcept { return {}; }
        
        void return_value(T val) {
            value = std::move(val);
        }
        
        void unhandled_exception() {
            exception = std::current_exception();
        }
    };
    
    std::coroutine_handle<promise_type> h;
    
    explicit Task(std::coroutine_handle<promise_type> handle) : h(handle) {}
    
    ~Task() {
        if (h)
            h.destroy();
    }
    
    // 移动构造
    Task(Task&& other) noexcept : h(other.h) {
        other.h = nullptr;
    }
    
    // Awaitable接口
    bool await_ready() {
        return h.done();
    }
    
    void await_suspend(std::coroutine_handle<> waiting) {
        h.promise().waiting = waiting;
    }
    
    T await_resume() {
        if (h.promise().exception) {
            std::rethrow_exception(h.promise().exception);
        }
        return std::move(h.promise().value);
    }
    
    // 同步获取结果
    T get() {
        if (!h.done()) {
            h.resume();
        }
        if (h.promise().exception) {
            std::rethrow_exception(h.promise().exception);
        }
        return std::move(h.promise().value);
    }
};

// void特化
template<>
struct Task<void> {
    struct promise_type {
        std::exception_ptr exception;
        
        Task get_return_object() {
            return Task{std::coroutine_handle<promise_type>::from_promise(*this)};
        }
        
        std::suspend_never initial_suspend() { return {}; }
        std::suspend_never final_suspend() noexcept { return {}; }
        
        void return_void() {}
        
        void unhandled_exception() {
            exception = std::current_exception();
        }
    };
    
    std::coroutine_handle<promise_type> h;
    
    explicit Task(std::coroutine_handle<promise_type> handle) : h(handle) {}
    
    ~Task() {
        if (h)
            h.destroy();
    }
    
    Task(Task&& other) noexcept : h(other.h) {
        other.h = nullptr;
    }
    
    void get() {
        if (!h.done()) {
            h.resume();
        }
        if (h.promise().exception) {
            std::rethrow_exception(h.promise().exception);
        }
    }
};

// 生成器协程
template<typename T>
struct Generator {
    struct promise_type {
        T current_value;
        
        Generator get_return_object() {
            return Generator{std::coroutine_handle<promise_type>::from_promise(*this)};
        }
        
        std::suspend_always initial_suspend() { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }
        
        std::suspend_always yield_value(T value) {
            current_value = std::move(value);
            return {};
        }
        
        void return_void() {}
        void unhandled_exception() { std::terminate(); }
    };
    
    std::coroutine_handle<promise_type> h;
    
    explicit Generator(std::coroutine_handle<promise_type> handle) : h(handle) {}
    
    ~Generator() {
        if (h)
            h.destroy();
    }
    
    Generator(Generator&& other) noexcept : h(other.h) {
        other.h = nullptr;
    }
    
    // 迭代器接口
    struct iterator {
        std::coroutine_handle<promise_type> h;
        
        iterator& operator++() {
            h.resume();
            return *this;
        }
        
        T& operator*() {
            return h.promise().current_value;
        }
        
        bool operator!=(std::default_sentinel_t) const {
            return !h.done();
        }
    };
    
    iterator begin() {
        h.resume();
        return {h};
    }
    
    std::default_sentinel_t end() {
        return {};
    }
};

// 示例1：简单的协程使用
Task<int> compute_async(int x, int y) {
    std::cout << "开始计算 " << x << " + " << y << std::endl;
    
    // 模拟异步操作
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    co_return x + y;
}

// 示例2：生成器模式
Generator<int> fibonacci(int n) {
    int a = 0, b = 1;
    for (int i = 0; i < n; ++i) {
        co_yield a;
        auto temp = a;
        a = b;
        b = temp + b;
    }
}

// 示例3：异步数据处理
Task<std::vector<int>> process_data_batch(const std::vector<int>& data) {
    std::vector<int> results;
    results.reserve(data.size());
    
    std::cout << "处理 " << data.size() << " 个数据项..." << std::endl;
    
    for (int val : data) {
        // 模拟异步处理
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        results.push_back(val * 2);
    }
    
    co_return results;
}

// 示例4：链式协程调用
Task<void> process_pipeline() {
    std::cout << "\n=== 管道处理示例 ===" << std::endl;
    
    // 第一步：生成数据
    std::vector<int> raw_data = {1, 2, 3, 4, 5};
    
    // 第二步：处理数据
    auto processed = co_await process_data_batch(raw_data);
    
    // 第三步：输出结果
    std::cout << "处理结果: ";
    for (int val : processed) {
        std::cout << val << " ";
    }
    std::cout << std::endl;
}

// 示例5：实际应用 - 模拟多个API调用
Task<int> fetch_from_api(const std::string& endpoint) {
    std::cout << "调用API: " << endpoint << std::endl;
    
    // 模拟网络延迟
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> delay_dist(100, 300);
    std::uniform_int_distribution<> result_dist(1, 100);
    
    std::this_thread::sleep_for(std::chrono::milliseconds(delay_dist(gen)));
    
    int result = result_dist(gen);
    std::cout << endpoint << " 返回: " << result << std::endl;
    
    co_return result;
}

// 并发执行多个任务
Task<void> concurrent_api_calls() {
    std::cout << "\n=== 并发API调用示例 ===" << std::endl;
    
    auto start = std::chrono::steady_clock::now();
    
    // 启动多个异步任务
    std::vector<std::thread> threads;
    std::vector<int> results(3);
    
    threads.emplace_back([&results]() {
        results[0] = fetch_from_api("/api/users").get();
    });
    
    threads.emplace_back([&results]() {
        results[1] = fetch_from_api("/api/posts").get();
    });
    
    threads.emplace_back([&results]() {
        results[2] = fetch_from_api("/api/comments").get();
    });
    
    // 等待所有线程完成
    for (auto& t : threads) {
        t.join();
    }
    
    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    int total = 0;
    for (int r : results) {
        total += r;
    }
    
    std::cout << "所有API调用完成，总计: " << total << std::endl;
    std::cout << "总耗时: " << duration.count() << "ms (并发执行)" << std::endl;
    
    co_return;
}

int main() {
    std::cout << "=== C++协程实用示例 ===" << std::endl;
    
    // 1. 简单计算
    {
        std::cout << "\n1. 简单异步计算:" << std::endl;
        auto task = compute_async(10, 20);
        int result = task.get();
        std::cout << "结果: " << result << std::endl;
    }
    
    // 2. 生成器
    {
        std::cout << "\n2. 斐波那契生成器:" << std::endl;
        std::cout << "前10个斐波那契数: ";
        for (int val : fibonacci(10)) {
            std::cout << val << " ";
        }
        std::cout << std::endl;
    }
    
    // 3. 管道处理
    {
        //process_pipeline().get();
    }
    
    // 4. 并发API调用
    {
        //concurrent_api_calls().get();
    }
    
    // 5. 对比顺序执行
    {
        std::cout << "\n=== 顺序API调用对比 ===" << std::endl;
        auto start = std::chrono::steady_clock::now();
        
        int r1 = fetch_from_api("/api/users").get();
        int r2 = fetch_from_api("/api/posts").get();
        int r3 = fetch_from_api("/api/comments").get();
        
        auto end = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        std::cout << "顺序执行总计: " << (r1 + r2 + r3) << std::endl;
        std::cout << "总耗时: " << duration.count() << "ms (顺序执行)" << std::endl;
    }
    
    std::cout << "\n程序结束" << std::endl;
    return 0;
}