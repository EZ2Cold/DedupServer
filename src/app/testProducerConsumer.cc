#include <iostream>
#include <pthread.h>
#include <queue>
using namespace std;

const int MAXITEMNUM = 100;
int count  = 0;
const int QUEUESIZE = 5;
queue<int> buffer;
pthread_mutex_t mutex;
pthread_cond_t cond_noempty, cond_nofull;

void* func_producer(void*) {
    printf("producer starts working.\n");
    while(true) {
        if(count == MAXITEMNUM) break;
        ++count;
        pthread_mutex_lock(&mutex);
        while(buffer.size() == QUEUESIZE) {
            pthread_cond_wait(&cond_nofull, &mutex);
        }
        buffer.push(count);
        printf("producer is putting %d\n", count);
        pthread_mutex_unlock(&mutex);
        pthread_cond_signal(&cond_noempty);
    }
    printf("producer exits. count = %d \n", count);
}

void* func_consumer(void*) {
    printf("consumer starts working.\n");
    while(true) {
        pthread_mutex_lock(&mutex);
        while(buffer.empty()) {
            pthread_cond_wait(&cond_noempty, &mutex);
        }
        int item = buffer.front();
        buffer.pop();
        printf("consumer is getting %d\n", item);
        pthread_mutex_unlock(&mutex);
        pthread_cond_signal(&cond_nofull);
        if(item == MAXITEMNUM)   break;
    }
    printf("consumer exits.\n");
}

int main() {
    pthread_mutex_init(&mutex, nullptr);
    pthread_cond_init(&cond_noempty, nullptr);
    pthread_cond_init(&cond_nofull, nullptr);

    pthread_t producer, consumer;
    pthread_create(&producer, nullptr, func_producer, nullptr);
    pthread_create(&consumer, nullptr, func_consumer, nullptr);

    pthread_exit(nullptr);
}