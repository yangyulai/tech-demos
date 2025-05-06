cmake_minimum_required(VERSION 3.10)
project(TaskManagerDemo LANGUAGES CXX)

# 使用 C++17
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 添加 src 目录为 include 路径
include_directories(${PROJECT_SOURCE_DIR}/src)

# 可执行目标，自动搜集 src 下的 .cpp（这里只列出 main.cpp）
add_executable(TaskManagerDemo
    src/main.cpp
)

# （可选）如果你把 TaskManager 实现拆到 .cpp，也可以：
# add_library(TaskManagerLib src/TaskManager.cpp)
# target_include_directories(TaskManagerLib PUBLIC src)
# target_link_libraries(TaskManagerDemo PRIVATE TaskManagerLib)
