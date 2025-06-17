#include <iostream>
#include <coroutine>
#include <thread>
#include <chrono>
#include <vector>
#include <memory>
#include <random>

// �򵥵�����Э��
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
    
    // �ƶ�����
    Task(Task&& other) noexcept : h(other.h) {
        other.h = nullptr;
    }
    
    // Awaitable�ӿ�
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
    
    // ͬ����ȡ���
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

// void�ػ�
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

// ������Э��
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
    
    // �������ӿ�
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

// ʾ��1���򵥵�Э��ʹ��
Task<int> compute_async(int x, int y) {
    std::cout << "��ʼ���� " << x << " + " << y << std::endl;
    
    // ģ���첽����
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    co_return x + y;
}

// ʾ��2��������ģʽ
Generator<int> fibonacci(int n) {
    int a = 0, b = 1;
    for (int i = 0; i < n; ++i) {
        co_yield a;
        auto temp = a;
        a = b;
        b = temp + b;
    }
}

// ʾ��3���첽���ݴ���
Task<std::vector<int>> process_data_batch(const std::vector<int>& data) {
    std::vector<int> results;
    results.reserve(data.size());
    
    std::cout << "���� " << data.size() << " ��������..." << std::endl;
    
    for (int val : data) {
        // ģ���첽����
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        results.push_back(val * 2);
    }
    
    co_return results;
}

// ʾ��4����ʽЭ�̵���
Task<void> process_pipeline() {
    std::cout << "\n=== �ܵ�����ʾ�� ===" << std::endl;
    
    // ��һ������������
    std::vector<int> raw_data = {1, 2, 3, 4, 5};
    
    // �ڶ�������������
    auto processed = co_await process_data_batch(raw_data);
    
    // ��������������
    std::cout << "������: ";
    for (int val : processed) {
        std::cout << val << " ";
    }
    std::cout << std::endl;
}

// ʾ��5��ʵ��Ӧ�� - ģ����API����
Task<int> fetch_from_api(const std::string& endpoint) {
    std::cout << "����API: " << endpoint << std::endl;
    
    // ģ�������ӳ�
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> delay_dist(100, 300);
    std::uniform_int_distribution<> result_dist(1, 100);
    
    std::this_thread::sleep_for(std::chrono::milliseconds(delay_dist(gen)));
    
    int result = result_dist(gen);
    std::cout << endpoint << " ����: " << result << std::endl;
    
    co_return result;
}

// ����ִ�ж������
Task<void> concurrent_api_calls() {
    std::cout << "\n=== ����API����ʾ�� ===" << std::endl;
    
    auto start = std::chrono::steady_clock::now();
    
    // ��������첽����
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
    
    // �ȴ������߳����
    for (auto& t : threads) {
        t.join();
    }
    
    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    int total = 0;
    for (int r : results) {
        total += r;
    }
    
    std::cout << "����API������ɣ��ܼ�: " << total << std::endl;
    std::cout << "�ܺ�ʱ: " << duration.count() << "ms (����ִ��)" << std::endl;
    
    co_return;
}

int main() {
    std::cout << "=== C++Э��ʵ��ʾ�� ===" << std::endl;
    
    // 1. �򵥼���
    {
        std::cout << "\n1. ���첽����:" << std::endl;
        auto task = compute_async(10, 20);
        int result = task.get();
        std::cout << "���: " << result << std::endl;
    }
    
    // 2. ������
    {
        std::cout << "\n2. 쳲�����������:" << std::endl;
        std::cout << "ǰ10��쳲�������: ";
        for (int val : fibonacci(10)) {
            std::cout << val << " ";
        }
        std::cout << std::endl;
    }
    
    // 3. �ܵ�����
    {
        //process_pipeline().get();
    }
    
    // 4. ����API����
    {
        //concurrent_api_calls().get();
    }
    
    // 5. �Ա�˳��ִ��
    {
        std::cout << "\n=== ˳��API���öԱ� ===" << std::endl;
        auto start = std::chrono::steady_clock::now();
        
        int r1 = fetch_from_api("/api/users").get();
        int r2 = fetch_from_api("/api/posts").get();
        int r3 = fetch_from_api("/api/comments").get();
        
        auto end = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        std::cout << "˳��ִ���ܼ�: " << (r1 + r2 + r3) << std::endl;
        std::cout << "�ܺ�ʱ: " << duration.count() << "ms (˳��ִ��)" << std::endl;
    }
    
    std::cout << "\n�������" << std::endl;
    return 0;
}