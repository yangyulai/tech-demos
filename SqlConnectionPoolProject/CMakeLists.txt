cmake_minimum_required(VERSION 3.10)

# 设置项目名称
project(SqlConnectionPoolProject)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)

# 查找 ODBC 库（假设系统中安装了 ODBC）
find_package(ODBC REQUIRED)

# 将源文件和头文件添加到目标
add_executable(SqlConnectionPoolApp main.cpp SqlConnectionPool.cpp)

# 链接 ODBC 库
target_link_libraries(SqlConnectionPoolApp PRIVATE ODBC::ODBC)

# 如果你使用了第三方库（例如 Boost），你可以使用类似以下的语句来链接：
# find_package(Boost REQUIRED)
# target_link_libraries(SqlConnectionPoolApp PRIVATE Boost::Boost)

# 其他设置（例如定义宏、包含目录等）
# include_directories(${PROJECT_SOURCE_DIR}/include)
