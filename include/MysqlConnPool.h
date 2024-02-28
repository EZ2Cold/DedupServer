#ifndef MYSQLCONNPOOL_H_
#define MYSQLCONNPOOL_H_
#include <stddef.h>
#include <list>
#include <mysql/mysql.h>
#include <mutex>
#include <condition_variable>
#include <pthread.h>
#include <memory>

class MysqlConnPool {
private:
    size_t m_maxconn;   // 最大连接数
    size_t m_freeconn;  // 当前空闲的连接数
    std::list<MYSQL*> m_connpool;   // 连接池
    std::mutex m_mtx;
    std::condition_variable m_cond_notEmpty;
public:
    std::string m_ip;
    unsigned int m_port;
    std::string m_user;
    std::string m_passwd;
    std::string m_database;

    MysqlConnPool(std::string ip, unsigned int port, std::string user, std::string passwd, std::string database, size_t maxconn);
    ~MysqlConnPool();
    MYSQL* get_connection();
    void release_connection(MYSQL* conn);
};

class MysqlConn {
private:
    MYSQL* m_conn;
    std::shared_ptr<MysqlConnPool> m_pool;
public:
    MysqlConn(std::shared_ptr<MysqlConnPool> pool);
    ~MysqlConn();
    bool hasResult(const char* query);  // return whether the query has the result
    void executeInsertion(const char* query);
};
#endif