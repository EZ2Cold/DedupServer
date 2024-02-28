#include <MysqlConnPool.h>
#include <Logger.h>
#include <unistd.h>

MysqlConnPool::MysqlConnPool(std::string ip, unsigned int port, std::string user, std::string passwd, std::string database, size_t maxconn) 
    :m_ip(ip), m_port(port), m_user(user), m_passwd(passwd), m_database(database), m_maxconn(maxconn) {
    m_freeconn = 0;

    for(int i = 0; i < m_maxconn; i++) {
        MYSQL* mysql = mysql_init(nullptr);
        if(nullptr == mysql) {
            LOG_ERROR("mysql_init() failed");
            exit(EXIT_FAILURE);
        }  
        if (!mysql_real_connect(mysql, m_ip.c_str(), m_user.c_str(), m_passwd.c_str(), m_database.c_str(), m_port, NULL, 0))
        {
            LOG_ERROR("connect mysql server failed");
            exit(EXIT_FAILURE);
        }
        m_connpool.push_back(mysql);
        m_freeconn++;
    }

}

MysqlConnPool::~MysqlConnPool() {
    for(auto it = m_connpool.begin(); it != m_connpool.end(); it++) {
        mysql_close(*it);
    }
}

MYSQL* MysqlConnPool::get_connection() {
    std::unique_lock<std::mutex> lck(m_mtx);
    while(m_freeconn == 0) {
        m_cond_notEmpty.wait(lck);
    }
    MYSQL* conn = m_connpool.front();
    m_connpool.pop_front();
    m_freeconn--;
    LOG_INFO("fetch a mysql connection from the pool");
    return conn;
}

void MysqlConnPool::release_connection(MYSQL* conn) {
    std::unique_lock<std::mutex> lck(m_mtx);
    for(auto it = m_connpool.begin(); it != m_connpool.end(); it++) {
        if(*it == conn)  {  // 重复释放
            lck.unlock();
            return;
        }
    }
    m_connpool.push_back(conn);
    m_freeconn++;
    m_cond_notEmpty.notify_one();
    LOG_INFO("put a mysql connection back into the pool");
}

MysqlConn::MysqlConn(std::shared_ptr<MysqlConnPool> pool) 
    :m_pool(pool) {
    m_conn = pool->get_connection();
}

MysqlConn::~MysqlConn() {
    m_pool->release_connection(m_conn);
}

bool MysqlConn::hasResult(const char* query) {
    mysql_query(m_conn, query);
    MYSQL_RES *result = mysql_store_result(m_conn);
    int num_rows = mysql_num_rows(result);
    mysql_free_result(result);
    return num_rows != 0;
}

void MysqlConn::executeInsertion(const char* query) {
    mysql_query(m_conn, query);
}