#include <iostream>
#include <coroutine>
#include <memory>
#include <thread>
#include <chrono>

// 1. 简单的生成器协程
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
            current_value = value;
            return {};
        }
        
        void return_void() {}
        void unhandled_exception() { std::terminate(); }
    };
    
    std::coroutine_handle<promise_type> handle;
    
    explicit Generator(std::coroutine_handle<promise_type> h) : handle(h) {}
    
    ~Generator() {
        if (handle) handle.destroy();
    }
    
    // 移动构造和赋值
    Generator(Generator&& other) noexcept : handle(other.handle) {
        other.handle = nullptr;
    }
    
    Generator& operator=(Generator&& other) noexcept {
        if (this != &other) {
            if (handle) handle.destroy();
            handle = other.handle;
            other.handle = nullptr;
        }
        return *this;
    }
    
    // 获取下一个值
    bool next() {
        handle.resume();
        return !handle.done();
    }
    
    T value() const {
        return handle.promise().current_value;
    }
};

// 使用生成器协程产生斐波那契数列
Generator<int> fibonacci(int n) {
    int a = 0, b = 1;
    for (int i = 0; i < n; ++i) {
        co_yield a;
        int temp = a;
        a = b;
        b = temp + b;
    }
}

// 2. 异步任务协程
struct Task {
    struct promise_type {
        Task get_return_object() {
            return Task{std::coroutine_handle<promise_type>::from_promise(*this)};
        }
        
        std::suspend_never initial_suspend() { return {}; }
        std::suspend_never final_suspend() noexcept { return {}; }
        
        void return_void() {}
        void unhandled_exception() { std::terminate(); }
    };
    
    std::coroutine_handle<promise_type> handle;
    
    explicit Task(std::coroutine_handle<promise_type> h) : handle(h) {}
};

// 自定义awaitable类型
struct AsyncTimer {
    std::chrono::milliseconds duration;
    
    bool await_ready() const noexcept { return false; }
    
    void await_suspend(std::coroutine_handle<> h) const {
        std::thread([h, this]() {
            std::this_thread::sleep_for(duration);
            h.resume();
        }).detach();
    }
    
    void await_resume() const noexcept {}
};

// 异步任务示例
Task asyncTask() {
    std::cout << "任务开始..." << std::endl;
    
    std::cout << "等待1秒..." << std::endl;
    co_await AsyncTimer{std::chrono::milliseconds(1000)};
    std::cout << "1秒后继续" << std::endl;
    
    std::cout << "再等待2秒..." << std::endl;
    co_await AsyncTimer{std::chrono::milliseconds(2000)};
    std::cout << "任务完成！" << std::endl;
}

// 3. 带返回值的协程
template<typename T>
struct Future {
    struct promise_type {
        std::optional<T> value;
        std::exception_ptr exception;
        
        Future get_return_object() {
            return Future{std::coroutine_handle<promise_type>::from_promise(*this)};
        }
        
        std::suspend_never initial_suspend() { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }
        
        void return_value(T val) {
            value = std::move(val);
        }
        
        void unhandled_exception() {
            exception = std::current_exception();
        }
    };
    
    std::coroutine_handle<promise_type> handle;
    
    explicit Future(std::coroutine_handle<promise_type> h) : handle(h) {}
    
    ~Future() {
        if (handle) handle.destroy();
    }
    
    T get() {
        if (!handle.done()) {
            handle.resume();
        }
        
        if (handle.promise().exception) {
            std::rethrow_exception(handle.promise().exception);
        }
        
        return *handle.promise().value;
    }
};

// 计算异步结果
Future<int> computeAsync(int x, int y) {
    co_await AsyncTimer{std::chrono::milliseconds(500)};
    co_return x + y;
}

int main() {
    // 1. 使用生成器
    std::cout << "=== 生成器示例 ===" << std::endl;
    auto gen = fibonacci(10);
    std::cout << "斐波那契数列前10个数: ";
    while (gen.next()) {
        std::cout << gen.value() << " ";
    }
    std::cout << std::endl << std::endl;
    
    // 2. 异步任务
    std::cout << "=== 异步任务示例 ===" << std::endl;
    auto task = asyncTask();
    
    // 主线程继续执行其他工作
    for (int i = 0; i < 5; ++i) {
        std::cout << "主线程工作中... " << i << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(700));
    }
    std::cout << std::endl;
    
    // 3. 带返回值的协程
    std::cout << "=== 带返回值的协程示例 ===" << std::endl;
    auto future = computeAsync(10, 20);
    std::cout << "开始计算..." << std::endl;
    int result = future.get();
    std::cout << "计算结果: " << result << std::endl;
    
    return 0;
}