#include <ThreadPool.h>
#include <iostream>

ThreadPool::ThreadPool(size_t thread_num, size_t queue_size)
    :m_thread_num(thread_num) {
    // create request queue
    m_requests = std::make_shared<BlockQueue<Request>>(queue_size);

    // create threads
    for(int i = 0; i < m_thread_num; i++) {
        auto th = std::make_shared<std::thread>(&ThreadPool::run, this);
        m_threads.push_back(th);
    }
}

ThreadPool::~ThreadPool() {
    m_requests->set_done();
    for(auto th : m_threads) {
        th->join();
    }
}

void ThreadPool::append(Request req) {
    m_requests->push(req);
}

void ThreadPool::run() {
    Request req;
    while(true) {
        if(!m_requests->pop(req))   return;
        req.func(req.arg);
    }
}
