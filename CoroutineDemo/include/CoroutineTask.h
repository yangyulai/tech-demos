#pragma once
#include <coroutine>
#include <exception>
#include <boost/asio/thread_pool.hpp>
#include <boost/asio/post.hpp>
#include <functional>
#include <queue>
#include <mutex>

// 主线程事件分发器
class MainThreadDispatcher {
public:
    static MainThreadDispatcher& instance() {
        static MainThreadDispatcher inst;
        return inst;
    }
    void post(std::function<void()> fn) {
        std::lock_guard lk(mutex_);
        queue_.push(std::move(fn));
    }
    void run_pending() {
        std::queue<std::function<void()>> q;
        {
            std::lock_guard lk(mutex_);
            std::swap(q, queue_);
        }
        while (!q.empty()) {
            q.front()();
            q.pop();
        }
    }
private:
    std::mutex mutex_;
    std::queue<std::function<void()>> queue_;
};

// 切到后台线程池的 Awaiter
struct ThreadPoolAwaiter {
    boost::asio::thread_pool& pool;
    bool await_ready() const noexcept { return false; }
    void await_suspend(std::coroutine_handle<> h) const {
        boost::asio::post(pool, [h](){ h.resume(); });
    }
    void await_resume() const noexcept {}
};

// 切回主线程的 Awaiter
struct MainThreadAwaiter {
    bool await_ready() const noexcept { return false; }
    void await_suspend(std::coroutine_handle<> h) const {
        MainThreadDispatcher::instance().post([h](){ h.resume(); });
    }
    void await_resume() const noexcept {}
};

// Task<T> 协程类型
template<typename T = void>
struct Task {
    struct promise_type {
        std::coroutine_handle<> continuation_;
        T value_;
        Task get_return_object() {
            return Task{ std::coroutine_handle<promise_type>::from_promise(*this) };
        }
        std::suspend_never initial_suspend() noexcept { return {}; }
        struct FinalAwaiter {
            bool await_ready() noexcept { return false; }
            void await_suspend(std::coroutine_handle<promise_type> h) noexcept {
                if (h.promise().continuation_)
                    h.promise().continuation_.resume();
            }
            void await_resume() noexcept {}
        };
        auto final_suspend() noexcept { return FinalAwaiter{}; }
        void return_value(T v) noexcept { value_ = std::move(v); }
        void unhandled_exception() { std::terminate(); }
    };
    using handle_type = std::coroutine_handle<promise_type>;
    handle_type handle_;
    explicit Task(handle_type h): handle_(h) {}
    Task(Task&& o) noexcept: handle_(o.handle_) { o.handle_ = {}; }
    ~Task() { if (handle_) handle_.destroy(); }
    void set_continuation(std::coroutine_handle<> c) { handle_.promise().continuation_ = c; }
    T get() { return std::move(handle_.promise().value_); }
};

// void 特化
template<>
struct Task<void> {
    struct promise_type {
        std::coroutine_handle<> continuation_;
        Task get_return_object() {
            return Task{ std::coroutine_handle<promise_type>::from_promise(*this) };
        }
        std::suspend_never initial_suspend() noexcept { return {}; }
        struct FinalAwaiter {
            bool await_ready() noexcept { return false; }
            void await_suspend(std::coroutine_handle<promise_type> h) noexcept {
                if (h.promise().continuation_)
                    h.promise().continuation_.resume();
            }
            void await_resume() noexcept {}
        };
        auto final_suspend() noexcept { return FinalAwaiter{}; }
        void return_void() noexcept {}
        void unhandled_exception() { std::terminate(); }
    };
    using handle_type = std::coroutine_handle<promise_type>;
    handle_type handle_;
    explicit Task(handle_type h): handle_(h) {}
    Task(Task&& o) noexcept: handle_(o.handle_) { o.handle_ = {}; }
    ~Task() { if (handle_) handle_.destroy(); }
    void set_continuation(std::coroutine_handle<> c) { handle_.promise().continuation_ = c; }
};

