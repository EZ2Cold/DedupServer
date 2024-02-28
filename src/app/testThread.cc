#include <thread>
#include <iostream>
#include <mutex>
class Hello {
public:
    Hello():m_cnt(0) {
        for(int i = 0; i < 10; i++) {
            std::thread t(&Hello::worker, this);
            t.detach();
        }
    }
private:
    void worker() {
        while(true) {
            std::lock_guard<std::mutex> lck(m_mtx);
            if(m_cnt == 100) {
                return;
            }
            m_cnt++;
            std::cout << std::this_thread::get_id() << " is increamenting. the value of counter is " << m_cnt << std::endl;
        }
    }
    int m_cnt;
    std::mutex m_mtx;
};
int main() {
    Hello hello;
    getchar();
    return 0;
}