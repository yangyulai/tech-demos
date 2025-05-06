#pragma once
#include <iostream>
#include <thread>
#include <future>
#include <mutex>
#include <condition_variable>
#include <unordered_map>
#include <queue>
#include <atomic>
#include <functional>
#include <type_traits>
#include <stdexcept>
#include <chrono>

class TaskManager {
public:
    // 构造函数：启动指定数量的工作线程（最大并发线程数）
    TaskManager(size_t maxThreads = std::thread::hardware_concurrency())
        : stop_(false), maxThreads_(maxThreads)
    {
        if (maxThreads_ == 0) maxThreads_ = 1;
        // 启动工作线程
        for (size_t i = 0; i < maxThreads_; ++i) {
            workers_.emplace_back([this] {
                while (true) {
                    std::function<void()> job;
                    {
                        std::unique_lock<std::mutex> lock(queueMutex_);
                        // 等待任务或停止信号
                        cv_.wait(lock, [this] {
                            return stop_ || !taskQueue_.empty();
                        });
                        if (stop_ && taskQueue_.empty()) {
                            // 若停止标志置位且无任务则退出线程
                            return;
                        }
                        // 从队列获取下一个任务
                        auto taskPair = std::move(taskQueue_.front());
                        taskQueue_.pop();
                        job = std::move(taskPair.second);
                    }
                    // 释放队列锁后执行任务
                    job();
                    // 任务执行完毕（此处可进行后续处理，例如通知完成）
                }
            });
        }
    }

    ~TaskManager() {
        // 发出停止信号，准备关闭线程池
        {
            std::unique_lock<std::mutex> lock(queueMutex_);
            stop_ = true;
        }
        cv_.notify_all();
        // 等待所有工作线程结束
        for (std::thread &worker : workers_) {
            if (worker.joinable()) {
                worker.join();
            }
        }
    }

    // 添加新任务，返回任务ID用于查询状态或取消任务
    template<typename F, typename... Args>
    int addTask(F&& f, Args&&... args) {
        using ReturnType = std::invoke_result_t<std::decay_t<F>, std::decay_t<Args>...>;
        // 生成任务ID
        int taskId = nextTaskId_++;

        // 创建任务函数绑定以及对应的 promise
        auto boundTask = std::bind(std::forward<F>(f), std::forward<Args>(args)...);
        auto cancelFlag = std::make_shared<std::atomic<bool>>(false);
        auto promisePtr = std::make_shared<std::promise<ReturnType>>();
        std::future<ReturnType> fut = promisePtr->get_future();
        std::shared_future<ReturnType> sharedFut = fut.share();

        // 将任务信息保存到任务表（线程安全）
        auto taskInfo = std::make_unique<TaskHolder<ReturnType>>(sharedFut, cancelFlag);
        {
            std::lock_guard<std::mutex> lock(tasksMutex_);
            tasks_.emplace(taskId, std::move(taskInfo));
        }

        // 封装实际执行的 job 函数，调用用户任务并设置 promise 结果
        auto job = [boundTask, promisePtr, cancelFlag]() {
            if (cancelFlag->load()) {
                // 若取消标志为 true，则设置异常状态以标识被取消
                promisePtr->set_exception(std::make_exception_ptr(std::runtime_error("Task cancelled")));
            } else {
                try {
                    if constexpr (std::is_void_v<ReturnType>) {
                        boundTask();
                        promisePtr->set_value();
                    } else {
                        promisePtr->set_value(boundTask());
                    }
                } catch (...) {
                    // 捕获任务执行中抛出的任何异常
                    promisePtr->set_exception(std::current_exception());
                }
            }
        };

        // 将 job 放入任务队列（线程安全）
        {
            std::lock_guard<std::mutex> lock(queueMutex_);
            if (stop_) {
                throw std::runtime_error("Cannot add new tasks after stopping");
            }
            taskQueue_.emplace(taskId, std::move(job));
        }
        cv_.notify_one();
        return taskId;
    }

    // 检查任务是否已完成（包含正常完成或被取消）
    bool isFinished(int taskId) {
        std::lock_guard<std::mutex> lock(tasksMutex_);
        auto it = tasks_.find(taskId);
        if (it == tasks_.end()) {
            return false; // 任务不存在或已被移除
        }
        return it->second->isDone();
    }

    // 取消一个正在执行或等待中的任务
    void cancelTask(int taskId) {
        std::lock_guard<std::mutex> lock(tasksMutex_);
        auto it = tasks_.find(taskId);
        if (it != tasks_.end()) {
            it->second->cancelFlag->store(true);
        }
        // 注意：若任务已在运行，需依靠任务代码自行检查取消标志:contentReference[oaicite:5]{index=5}
        // 若任务还在队列中，工作线程在执行前会发现取消标志从而跳过执行
    }

    // 获取任务结果；若任务抛异常或被取消，将在此重新抛出异常
    // 提取结果后，从任务表中移除该任务
    template<typename R>
    R getResult(int taskId) {
        std::unique_lock<std::mutex> lock(tasksMutex_);
        auto it = tasks_.find(taskId);
        if (it == tasks_.end()) {
            throw std::runtime_error("Task ID not found");
        }
        // 向下转型到具体的 TaskHolder 类型以访问对应的 future
        auto taskPtr = dynamic_cast<TaskHolder<R>*>(it->second.get());
        if (!taskPtr) {
            throw std::runtime_error("Result type mismatch for task ID");
        }
        // 如果任务尚未完成，这里先等待完成
        lock.unlock();
        taskPtr->future.wait();
        lock.lock();
        // 获取结果（由于已经等待完成，不会因为未就绪而异常）
        R result = taskPtr->future.get();
        // 从任务表中移除该任务释放资源
        tasks_.erase(it);
        return result;
    }

private:
    // 任务信息的基类，用于存储不同类型的任务结果
    struct TaskBase {
        std::shared_ptr<std::atomic<bool>> cancelFlag;
        TaskBase(std::shared_ptr<std::atomic<bool>> flag) : cancelFlag(flag) {}
        virtual ~TaskBase() = default;
        virtual bool isDone() const = 0;
    };
    // 模板派生类，保存特定类型任务的 future 和取消标志
    template<typename R>
    struct TaskHolder : public TaskBase {
        std::shared_future<R> future;
        TaskHolder(std::shared_future<R> fut, std::shared_ptr<std::atomic<bool>> flag)
            : TaskBase(flag), future(std::move(fut)) {}
        bool isDone() const override {
            // 检查 future 是否已就绪（非阻塞检查）
            return future.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
        }
    };

    std::unordered_map<int, std::unique_ptr<TaskBase>> tasks_;
    std::queue<std::pair<int, std::function<void()>>> taskQueue_;
    std::vector<std::thread> workers_;
    std::mutex tasksMutex_;
    std::mutex queueMutex_;
    std::condition_variable cv_;
    std::atomic<bool> stop_;
    size_t maxThreads_;
    std::atomic<int> nextTaskId_{0};
};
