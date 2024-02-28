#ifndef DEDUPSERVER_H_
#define DEDUPSERVER_H_
#include <stddef.h>
#include <ThreadPool.h>
#include <sys/epoll.h>
#include <map>
#include <set>
#include <ClientConn.h>
#include <mutex>
#include <MysqlConnPool.h>

class DedupServer {
public:
    DedupServer(int port, size_t thread_num, size_t queue_size);
    ~DedupServer();
    void init();
    void run();
    static void* stub(void* arg);
    bool isFileUnique(const int client_id, const std::string file_name);
    bool isFileExist(const int client_id, const std::string file_name);
    void addFile(const int client_id, const std::string file_name);
    void addChunk(std::pair<std::string, ChunkAddr_t> chunk);
    bool isChunkStored(std::string strChunkHash, ChunkAddr_t& chunkAddr);
public:
    int m_port; // 监听的端口号
    ThreadPool* m_threadPool;   // 线程池
    size_t m_thread_num; // 线程池大小
    size_t m_queue_size; // 任务队列的大小
    int m_listenfd; // 监听socket
    int m_epollfd;
    epoll_event* m_events;
    std::map<int, ClientConn*> m_clients; // 用户连接集合
    std::set<std::string> m_received_files; // 所有接收到的文件集合
    std::shared_ptr<std::map<std::string, ChunkAddr_t>> m_all_chunks;
    std::mutex m_mutex;
    std::shared_ptr<MysqlConnPool> m_mysqlPool;    // mysql 连接池
};

#endif