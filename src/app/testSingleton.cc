#include <iostream>
#include <unistd.h>
#include <thread>
#include <mutex>
#include <memory>
using namespace std;

class Single {
public:
    typedef shared_ptr<Single> Ptr;
private:
    Single() {
        cout << "构造函数" << endl;
    }
    // static Ptr single;
    static Single single;
    static mutex mtx;
public:
    // 删除拷贝构造函数和以及拷贝赋值运算符
    Single(const Single&) = delete;
    Single& operator=(const Single&) = delete;

    ~Single() {
        cout << "析构函数" << endl;
    }
    // 1. 双检测法
    // static Single::Ptr getInstance() {
    //     if(single == nullptr) {
    //         lock_guard<mutex> lk(mtx);  // lock_guard 自动加解锁
    //         if(single == nullptr) {
    //             single = Ptr(new Single);
    //         }
    //     }
    //     return single;
    // }

    // 2. 局部静态变量法
    // static Single& getInstance() {
    //     static Single single;
    //     return single;
    // }

    // 3. 饿汉模式
    static Single& getInstance() {
        return single;
    }
    
    void Print() {
        cout << "我的内存地址是: " << this << endl;
    }
};

void test() {
    sleep(1);
    // Single::Ptr p = Single::getInstance();
    Single& p = Single::getInstance();
    p.Print();
}

// Single::Ptr Single::single = nullptr;

Single Single::single;
mutex Single::mtx;

int main() {
    std::thread T[10];
    for(auto& i:T) {
        i = std::thread(test);
    }

    for(auto& i : T) {
        if(i.joinable()) {
            i.join();
        }
    }
    return 0;
}