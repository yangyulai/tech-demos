#include "SqlConnectionPool.h"
#include <iostream>

SqlConnection::SqlConnection(const std::wstring& connStr) {
    SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &env_);
    SQLSetEnvAttr(env_, SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3, 0);
    SQLAllocHandle(SQL_HANDLE_DBC, env_, &dbc_);
    
    SQLWCHAR outConnStr[1024]; SQLSMALLINT outLen;
    SQLRETURN ret = SQLDriverConnect(dbc_, NULL, (SQLWCHAR*)connStr.c_str(), SQL_NTS, 
                                     outConnStr, 1024, &outLen, SQL_DRIVER_NOPROMPT);
    if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
        throw std::runtime_error("Failed to connect to DB");
    }
}

SqlConnection::~SqlConnection() {
    if (dbc_ != SQL_NULL_HDBC) {
        SQLDisconnect(dbc_);
        SQLFreeHandle(SQL_HANDLE_DBC, dbc_);
    }
    if (env_ != SQL_NULL_HENV) {
        SQLFreeHandle(SQL_HANDLE_ENV, env_);
    }
}

bool SqlConnection::execute(const std::wstring& sql) {
    SQLHSTMT hstmt;
    if (SQLAllocHandle(SQL_HANDLE_STMT, dbc_, &hstmt) != SQL_SUCCESS) return false;
    SQLRETURN ret = SQLExecDirect(hstmt, (SQLWCHAR*)sql.c_str(), SQL_NTS);
    SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
    return (ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO);
}

SqlConnectionPool::SqlConnectionPool(size_t maxConn, const std::wstring& connStr, size_t initSize)
    : maxConnections(maxConn), connectionString(connStr) {
    for (size_t i = 0; i < initSize; ++i) {
        idleList.push_back(new SqlConnection(connectionString));
        ++currentCount;
    }
}

SqlConnectionPool::~SqlConnectionPool() {
    for (SqlConnection* conn : idleList) {
        delete conn;
    }
}

std::shared_ptr<SqlConnection> SqlConnectionPool::getConnection(int timeoutMs) {
    std::unique_lock<std::mutex> lock(poolMutex);

    if (idleList.empty() && currentCount < maxConnections) {
        try {
            idleList.push_back(new SqlConnection(connectionString));
            ++currentCount;
        } catch (...) {
            return nullptr; // 创建新连接失败
        }
    }

    if (idleList.empty()) {
        if (timeoutMs > 0) {
            if (cond.wait_for(lock, std::chrono::milliseconds(timeoutMs), [this]{ return !idleList.empty(); }) == false) {
                return nullptr;
            }
        } else {
            cond.wait(lock, [this]{ return !idleList.empty(); });
        }
    }

    SqlConnection* connPtr = idleList.front();
    idleList.pop_front();

    std::shared_ptr<SqlConnection> connHandle(connPtr, [this](SqlConnection* pConn) {
        std::lock_guard<std::mutex> guard(poolMutex);
        idleList.push_back(pConn);
        cond.notify_one();
    });
    return connHandle;
}
