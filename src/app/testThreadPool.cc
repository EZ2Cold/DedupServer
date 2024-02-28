#include <iostream>
#include <ThreadPool.h>
#include <unistd.h>
void* PrintHello(void* arg) {
    std::cout << "thread " << pthread_self() << " is handling a task" << std::endl;
    sleep((long)arg + 4);
    std::cout << "thread " << pthread_self() << " is returning" << std::endl;
    return arg;
}
int main() {
    ThreadPool thread_pool(8, 20);
    ThreadPool::Request req;
    req.func = PrintHello;
    sleep(5);
    for(int i = 0; i < 8; i++) {
        req.arg = (void*)i;
        thread_pool.append(req);
    }
    while(1);
    return 0;
}