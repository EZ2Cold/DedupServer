#include <iostream>
#include <mutex>
#include <condition_variable>
#include <queue>
using namespace std;

const int MAXITEMNUM = 100;
int count  = 0;
const int QUEUESIZE = 5;
queue<int> buffer;
std::mutex mu;
std::condition_variable cond_notEmpty, cond_notFull;

void* func_producer(void*) {
    printf("producer starts working.\n");
    while(true) {
        if(count == MAXITEMNUM) break;
        ++count;
        std::unique_lock<std::mutex> lck(mu);
        while(buffer.size() == QUEUESIZE) {
            cond_notFull.wait(lck);
        }
        buffer.push(count);
        printf("producer is putting %d\n", count);
        cond_notEmpty.notify_one();
    }
    printf("producer exits. count = %d \n", count);
}

void* func_consumer(void*) {
    printf("consumer starts working.\n");
    while(true) {
        std::unique_lock<std::mutex> lck(mu);
        while(buffer.empty()) {
            cond_notEmpty.wait(lck);
        }
        int item = buffer.front();
        buffer.pop();
        printf("consumer is getting %d\n", item);
        cond_notFull.notify_one();
        if(item == MAXITEMNUM)   break;
    }
    printf("consumer exits.\n");
}

int main() {
    pthread_t producer, consumer;
    pthread_create(&producer, nullptr, func_producer, nullptr);
    pthread_create(&consumer, nullptr, func_consumer, nullptr);

    pthread_exit(nullptr);
}