#include <iostream>
#include <MysqlConnPool.h>
#include <Logger.h>
#include <unistd.h>
std::shared_ptr<MysqlConnPool> pool;

void* worker(void* arg) {
    MysqlConn conn(pool);
    sleep(1);  
    return arg;
}
int main() {
    Logger::get_instance().init("logs/testMysqlConnPool");

    pool = std::make_shared<MysqlConnPool>("localhost", 3306, "bluesky", "alqoZM123_", "dedup_server", 8);
    pthread_t pid;
    for(int i = 0; i < 20; i++) {
        pthread_create(&pid, nullptr, worker, nullptr);
    }
    pthread_exit(nullptr);
}