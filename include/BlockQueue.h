#ifndef BLOCKQUEUE_H_
#define BLOCKQUEUE_H_
#include<queue>
#include<memory>
#include<stdio.h>
#include<mutex>
#include<condition_variable>
template <typename T>
class BlockQueue {
private:
    std::queue<T> m_queue; // 缓冲区
    size_t m_size;  // 缓冲队列长度
    std::mutex m_mtx;  // 互斥锁用于同步缓冲区
    std::condition_variable m_cond_notEmpty; // 队列非空时通知可以读
    std::condition_variable m_cond_notFull; // 队列非满时可以添加
    bool m_isEmpty; // 指示当前队列状态
    bool m_done;    // 指示任务是否完成
public:
    BlockQueue(size_t size) 
        :m_size(size), m_isEmpty(true), m_done(false) {
    }
    ~BlockQueue() {
    }
    void push(const T& e) {
        std::unique_lock<std::mutex> lck(m_mtx);
        while(m_queue.size() == m_size) {   
            m_cond_notFull.wait(lck);
        }
        m_queue.push(e);
        m_isEmpty = false;
        m_cond_notEmpty.notify_one();
    }

    bool pop(T& e) {
        std::unique_lock<std::mutex> lck(m_mtx);
        while(m_queue.empty()) {
            if(m_done)  return false;
            m_cond_notEmpty.wait_for(lck, std::chrono::seconds(1));
        }
        e = m_queue.front();
        m_queue.pop();
        if(m_queue.empty()) m_isEmpty = true;
        m_cond_notFull.notify_one();
        return true;
    }

    bool empty() {
        return m_isEmpty;
    }

    void set_done() {
        m_done = true;
    }
    void undone() {
        m_done = false;
    }
};

#endif