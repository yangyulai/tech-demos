cmake_minimum_required(VERSION 3.10)
project(Win32UIDemo LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(Win32UIDemo src/main.cpp)

# 链接公用控件库
target_link_libraries(Win32UIDemo PRIVATE comctl32)
