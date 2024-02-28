#ifndef THREADPOOL_H_
#define THREADPOOL_H_
#include <vector>
#include <BlockQueue.h>
#include <thread>
class ThreadPool {
public:
    typedef struct {
        void* (*func)(void*);
        void* arg;
    } Request;
private:
    std::vector<std::shared_ptr<std::thread>> m_threads; // 线程池
    std::shared_ptr<BlockQueue<Request>> m_requests;  // 任务请求队列
    size_t m_thread_num;    // 线程数

public:
    ThreadPool(size_t thread_num = 8, size_t queue_size = 20);
    ~ThreadPool();
    void append(Request req);
private:
    void run();
};

#endif