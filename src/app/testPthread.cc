#include <iostream>
#include <pthread.h>

using namespace std;

void* PrintHello(void* arg) {
    long tid = (long)arg;
    printf("Hello World! My id is %d!\n", tid);
    pthread_exit(arg);
}
#define THREADNUM 5
int main() {
    pthread_t threads[THREADNUM];
    pthread_attr_t attrs;
    void* status;

    pthread_attr_init(&attrs);
    pthread_attr_setdetachstate(&attrs, PTHREAD_CREATE_JOINABLE);

    for(int i = 0; i < THREADNUM; i++) {
        printf("Main: creating thread %d\n", i);
        int ret = pthread_create(&threads[i], &attrs, PrintHello, (void*)i);
        if(ret != 0) {
            perror("pthread_create()");
        }
    }

    pthread_attr_destroy(&attrs);

    for(int i = 0; i < THREADNUM; i++) {
        pthread_join(threads[i], &status);
        printf("thread %d completes. the result is %d\n", i, (long)status);
    }
    pthread_exit(nullptr);
}