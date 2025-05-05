#ifndef SQL_CONNECTION_POOL_H
#define SQL_CONNECTION_POOL_H
#include <Windows.h>

#include <sql.h>
#include <sqlext.h>
#include <deque>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <string>

class SqlConnection {
public:
    SqlConnection(const std::wstring& connStr);
    ~SqlConnection();
    bool execute(const std::wstring& sql);

private:
    SQLHENV env_{SQL_NULL_HENV};
    SQLHDBC dbc_{SQL_NULL_HDBC};
};

class SqlConnectionPool {
public:
    SqlConnectionPool(size_t maxConn, const std::wstring& connStr, size_t initSize=1);
    ~SqlConnectionPool();
    std::shared_ptr<SqlConnection> getConnection(int timeoutMs = 0);

private:
    size_t maxConnections;
    size_t currentCount = 0;
    std::wstring connectionString;
    std::deque<SqlConnection*> idleList;
    std::mutex poolMutex;
    std::condition_variable cond;
};

#endif // SQL_CONNECTION_POOL_H
