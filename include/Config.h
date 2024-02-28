#ifndef CONFIG_H_
#define CONFIG_H_
#include <stddef.h>
#include <string>
#include <define.h>

const size_t CHUNKSIZE = 4 << 10;  // 块大小 4KB
const size_t BATCHSIZE = 1024; // 每一批的个数
const HashType HASHTYPE = SHA256;  // 计算指纹的方式
const size_t HASHSIZE = 32;
const size_t CONTAINERSIZE = 4 << 20;   // container的大小
const bool CLOSELOG = false;    // 关闭日志
const size_t LOGLINES = 1000;    // 但文件日志行数

class ServerConfig{
public:
    int m_port = 12345; // server监听的端口号

    size_t m_thread_num = 8;    // 线程池大小
    size_t m_queue_size = 20;  // 任务队列的大小


    void parse_args(int argc, char* argv[]);
};

class ClientConfig {
public:
    const char* m_server_addr = "127.0.0.1";    // server 的IP地址
    int m_port = 12345; //   server的端口号

    char m_op;  // 操作：上传/下载
    std::string m_file_path;    // 待上传/下载文件路径 
    int m_client_id;    // 用户id
    void parse_args(int argc, char* argv[]);

    ~ClientConfig();
};


#endif