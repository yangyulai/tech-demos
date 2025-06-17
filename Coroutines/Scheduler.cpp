#include <iostream>
#include <coroutine>
#include <thread>
#include <chrono>
#include <vector>
#include <queue>
#include <functional>
#include <memory>
#include <random>
#include <sstream>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <optional>

// 1. Э�̵����� - �������첽ִ��
class Scheduler {
private:
    std::queue<std::coroutine_handle<>> ready_queue;
    std::vector<std::thread> workers;
    std::atomic<bool> running{ true };
    std::mutex queue_mutex;
    std::condition_variable cv;

public:
    static Scheduler& instance() {
        static Scheduler scheduler;
        return scheduler;
    }

    Scheduler() {
        // ���������߳�
        unsigned int thread_count = std::thread::hardware_concurrency();
        for (unsigned int i = 0; i < thread_count; ++i) {
            workers.emplace_back([this] { work_loop(); });
        }
    }

    ~Scheduler() {
        running = false;
        cv.notify_all();
        for (auto& t : workers) {
            t.join();
        }
    }

    void schedule(std::coroutine_handle<> handle) {
        {
            std::lock_guard<std::mutex> lock(queue_mutex);
            ready_queue.push(handle);
        }
        cv.notify_one();
    }

private:
    void work_loop() {
        while (running) {
            std::unique_lock<std::mutex> lock(queue_mutex);
            cv.wait(lock, [this] { return !ready_queue.empty() || !running; });

            if (!running) break;

            if (!ready_queue.empty()) {
                auto handle = ready_queue.front();
                ready_queue.pop();
                lock.unlock();

                handle.resume();
            }
        }
    }
};

// 2. �������첽Task - ֧��void�ͷ�void��������
template<typename T = void>
struct AsyncTask {
    struct promise_type {
        std::optional<T> value;
        std::exception_ptr exception;
        std::coroutine_handle<> waiting_coroutine;

        AsyncTask get_return_object() {
            return AsyncTask{ std::coroutine_handle<promise_type>::from_promise(*this) };
        }

        std::suspend_always initial_suspend() { return {}; }

        struct final_awaiter {
            bool await_ready() noexcept { return false; }
            void await_suspend(std::coroutine_handle<promise_type> h) noexcept {
                if (h.promise().waiting_coroutine) {
                    Scheduler::instance().schedule(h.promise().waiting_coroutine);
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

    std::coroutine_handle<promise_type> handle;

    explicit AsyncTask(std::coroutine_handle<promise_type> h) : handle(h) {
        // ��������ִ��
        Scheduler::instance().schedule(handle);
    }

    ~AsyncTask() {
        if (handle) handle.destroy();
    }

    // �ƶ�����
    AsyncTask(AsyncTask&& other) noexcept : handle(other.handle) {
        other.handle = nullptr;
    }

    // Awaiter�ӿ� - ����co_await
    bool await_ready() {
        return handle.done();
    }

    void await_suspend(std::coroutine_handle<> waiting) {
        handle.promise().waiting_coroutine = waiting;
    }

    T await_resume() {
        if (handle.promise().exception) {
            std::rethrow_exception(handle.promise().exception);
        }
        return *handle.promise().value;
    }
};

// AsyncTask<void> ���ػ��汾
template<>
struct AsyncTask<void> {
    struct promise_type {
        std::exception_ptr exception;
        std::coroutine_handle<> waiting_coroutine;

        AsyncTask get_return_object() {
            return AsyncTask{ std::coroutine_handle<promise_type>::from_promise(*this) };
        }

        std::suspend_always initial_suspend() { return {}; }

        struct final_awaiter {
            bool await_ready() noexcept { return false; }
            void await_suspend(std::coroutine_handle<promise_type> h) noexcept {
                if (h.promise().waiting_coroutine) {
                    Scheduler::instance().schedule(h.promise().waiting_coroutine);
                }
            }
            void await_resume() noexcept {}
        };

        final_awaiter final_suspend() noexcept { return {}; }

        void return_void() {}

        void unhandled_exception() {
            exception = std::current_exception();
        }
    };

    std::coroutine_handle<promise_type> handle;

    explicit AsyncTask(std::coroutine_handle<promise_type> h) : handle(h) {
        Scheduler::instance().schedule(handle);
    }

    ~AsyncTask() {
        if (handle) handle.destroy();
    }

    AsyncTask(AsyncTask&& other) noexcept : handle(other.handle) {
        other.handle = nullptr;
    }

    bool await_ready() {
        return handle.done();
    }

    void await_suspend(std::coroutine_handle<> waiting) {
        handle.promise().waiting_coroutine = waiting;
    }

    void await_resume() {
        if (handle.promise().exception) {
            std::rethrow_exception(handle.promise().exception);
        }
    }
};

// 3. �첽I/Oģ��
struct AsyncIO {
    static AsyncTask<std::string> read_file(const std::string& filename) {
        // ģ���첽�ļ���ȡ
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        co_return "File content of " + filename;
    }

    static AsyncTask<int> fetch_data(const std::string& url) {
        // ģ����������
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(1, 100);
        co_return dis(gen);
    }
};

// 4. ����ִ�ж���첽����
template<typename T>
AsyncTask<std::vector<T>> when_all(std::vector<AsyncTask<T>> tasks) {
    std::vector<T> results;
    results.reserve(tasks.size());

    for (auto& task : tasks) {
        results.push_back(co_await std::move(task));
    }

    co_return results;
}

// 5. ʵ��Ӧ��ʾ��

// ����������API����
AsyncTask<void> process_multiple_requests() {
    std::cout << "[" << std::this_thread::get_id() << "] ��ʼ����������..." << std::endl;

    // ��������첽����
    std::vector<AsyncTask<int>> tasks;
    for (int i = 0; i < 5; ++i) {
        tasks.push_back(AsyncIO::fetch_data("api/data/" + std::to_string(i)));
    }

    // �ȴ������������
    auto results = co_await when_all(std::move(tasks));

    int sum = 0;
    for (auto val : results) {
        sum += val;
    }

    std::cout << "[" << std::this_thread::get_id() << "] ����������ɣ��ܺ�: " << sum << std::endl;
}

// �ܵ�ʽ���ݴ���
template<typename T>
struct Stream {
    struct promise_type {
        std::optional<T> current_value;

        Stream get_return_object() {
            return Stream{ std::coroutine_handle<promise_type>::from_promise(*this) };
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

    Stream(std::coroutine_handle<promise_type> h) : handle(h) {}
    ~Stream() { if (handle) handle.destroy(); }

    std::optional<T> next() {
        if (!handle || handle.done()) return std::nullopt;
        handle.resume();
        if (handle.done()) return std::nullopt;
        return handle.promise().current_value;
    }
};

// �첽����������
Stream<int> async_data_producer() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1, 100);

    for (int i = 0; i < 10; ++i) {
        // ģ���첽��������
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        co_yield dis(gen);
    }
}

// 6. ��ʵ������Web������������
struct HttpRequest {
    std::string path;
    std::string method;
};

struct HttpResponse {
    int status_code;
    std::string body;
};

AsyncTask<HttpResponse> handle_request(HttpRequest request) {
    std::cout << "[" << std::this_thread::get_id() << "] ��������: "
        << request.method << " " << request.path << std::endl;

    if (request.path == "/users") {
        // ������ѯ�������Դ
        auto user_data = AsyncIO::fetch_data("database/users");
        auto cache_data = AsyncIO::fetch_data("cache/users");

        // ͬʱ�ȴ���������
        std::vector<AsyncTask<int>> tasks;
        tasks.push_back(std::move(user_data));
        tasks.push_back(std::move(cache_data));

        auto results = co_await when_all(std::move(tasks));

        std::stringstream response;
        response << "Users: " << results[0] << ", Cached: " << results[1];

        co_return HttpResponse{ 200, response.str() };
    }

    co_return HttpResponse{ 404, "Not Found" };
}

// 7. ������ʽ����
AsyncTask<int> compute_step1(int input) {
    std::cout << "[Step1] ����: " << input << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    co_return input * 2;
}

AsyncTask<int> compute_step2(int input) {
    std::cout << "[Step2] ����: " << input << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    co_return input + 10;
}

AsyncTask<int> compute_step3(int input) {
    std::cout << "[Step3] ����: " << input << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    co_return input / 2;
}

AsyncTask<void> chained_computation() {
    std::cout << "��ʼ��ʽ����..." << std::endl;

    // ��ʽ���ö���첽����
    int result = co_await compute_step1(5);
    result = co_await compute_step2(result);
    result = co_await compute_step3(result);

    std::cout << "���ս��: " << result << std::endl;
}

// ������չʾʵ��Ӧ��
int main() {
    std::cout << "=== ʵ�õ�Э��ʾ�� ===" << std::endl;
    std::cout << "���߳�ID: " << std::this_thread::get_id() << std::endl << std::endl;

    // 1. �������������󣨷�������
    {
        std::cout << "1. ��������ʾ��:" << std::endl;
        auto task = process_multiple_requests();

        // ���߳̿��Լ�������������
        for (int i = 0; i < 3; ++i) {
            std::cout << "[���߳�] ������������ " << i << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(150));
        }
    }

    // 2. ��ʽ���ݴ���
    {
        std::cout << "\n2. ��ʽ���ݴ���:" << std::endl;
        auto stream = async_data_producer();

        // �첽��������
        std::thread consumer([&stream]() {
            while (auto value = stream.next()) {
                std::cout << "[������] �յ�����: " << *value << std::endl;
            }
            });

        // ���̼߳�����������
        std::cout << "[���߳�] ����������������..." << std::endl;
        consumer.join();
    }

    // 3. Web������������
    {
        std::cout << "\n3. Web������������:" << std::endl;
        std::vector<AsyncTask<HttpResponse>> requests;

        // ģ������������
        requests.push_back(handle_request({ "/users", "GET" }));
        requests.push_back(handle_request({ "/posts", "GET" }));
        requests.push_back(handle_request({ "/users", "GET" }));

        // �ȴ������������
        auto responses = when_all(std::move(requests));

        // ����when_all����AsyncTask��������Ҫ�ȴ���
        std::thread waiter([&responses]() mutable {
            auto results = responses.await_resume();
            for (const auto& resp : results) {
                std::cout << "��Ӧ: " << resp.status_code << " - " << resp.body << std::endl;
            }
            });
        waiter.join();
    }

    // 4. ��ʽ������
    {
        std::cout << "\n4. ��ʽ������:" << std::endl;
        auto task = chained_computation();
        // ���̼߳�������
        std::cout << "[���߳�] ִ����������..." << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
    std::cout << "\n����������ɣ�" << std::endl;
    return 0;
}