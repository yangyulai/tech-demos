#include "TaskManager.h"
#include <vector>
int main() {
    TaskManager tm(4);  // 最多 4 个并发线程

    std::vector<int> taskIds;

    // 初始动态添加 5 个任务
    for (int i = 0; i < 5; ++i) {
        int id = tm.addTask([i]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(100 * (i + 1)));
            return i * i;
            });
        taskIds.push_back(id);
        std::cout << "Added task " << id << std::endl;
    }

    // 主循环：轮询、取结果、动态添加
    int loopCount = 0;
    while (true) {
        // 轮询当前所有任务
        if (!taskIds.empty())
        {
            for (auto it = taskIds.begin(); it != taskIds.end(); /*no increment*/) {
                int id = *it;
                if (tm.isFinished(id)) {
                    try {
                        // 因为所有这些任务返回 int，就用 int 作模板参数
                        int result = tm.getResult<int>(id);
                        std::cout << "[Result] task " << id << " = " << result << std::endl;
                    }
                    catch (const std::exception& ex) {
                        std::cout << "[Exception] task " << id << ": " << ex.what() << std::endl;
                    }
                    // 从列表中移除已处理的 ID
                    it = taskIds.erase(it);
                }
                else {
                    ++it;
                }
            }


        }
        // 每 20 次循环，再动态添加一个新任务
        if (++loopCount % 3 == 0) {
            int newId = tm.addTask([]() {
                std::this_thread::sleep_for(std::chrono::milliseconds(300));
                return 42;
                });
            taskIds.push_back(newId);
            std::cout << "Dynamically added task " << newId << std::endl;
        }

        // 主线程做其他工作
        std::cout << "Main thread working..." << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    std::cout << "All tasks completed or cancelled, exiting." << std::endl;
    return 0;
}
