#include "SqlConnectionPool.h"
#include <iostream>

int main() {
    std::wstring connStr = L"Driver={ODBC Driver 17 for SQL Server};Server=127.0.0.1;Database=svrconfig;Uid=fz536;Pwd=S9xiU93xO5fY3pG;";
    SqlConnectionPool pool(10, connStr, 2); // 最大10个连接，初始创建2个

    {
        auto conn = pool.getConnection(5000); // 等待最多5秒
        if (!conn) {
            std::wcerr << L"获取连接失败！" << std::endl;
            return -1;
        }
        bool success = conn->execute(L"SELECT * FROM svrconfig;");
        if (!success) {
            std::wcerr << L"查询执行失败！" << std::endl;
        }
    }

    std::thread t1([&pool]{
        auto conn = pool.getConnection();
        // ... 使用连接进行数据库操作
    });

    std::thread t2([&pool]{
        auto conn = pool.getConnection();
        // ... 使用连接进行数据库操作
    });

    t1.join();
    t2.join();

    return 0;
}
