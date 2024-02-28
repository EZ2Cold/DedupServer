#ifndef LOGGER_H_
#define LOGGER_H_
#include <BlockQueue.h>
#include <fstream>
#include <mutex>
#include <thread>

#define LOGBUFSIZE 1024

#define LOG_DEBUG(fmt, ...) Logger::get_instance().write_log(Logger::DEBUG, fmt, ##__VA_ARGS__);
#define LOG_INFO(fmt, ...) Logger::get_instance().write_log(Logger::INFO, fmt, ##__VA_ARGS__);
#define LOG_WARN(fmt, ...) Logger::get_instance().write_log(Logger::WARN, fmt, ##__VA_ARGS__);
#define LOG_ERROR(fmt, ...) Logger::get_instance().write_log(Logger::ERROR, fmt, ##__VA_ARGS__);

class Logger {
private:
    Logger() = default;
    void async_write_log();
    void flush();
public:
    // 定义日志级别
    typedef enum {
        DEBUG,
        INFO,
        WARN,
        ERROR
    }Level;
    ~Logger();
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    
    void init(const char* file_path, bool close_log = false, size_t split_lines = 10000, size_t queue_size = 0);
    void write_log(Level level, const char* fmt, ...);

    static Logger& get_instance() {
        static Logger instance;
        return instance;
    }
private:
    std::string m_dir_name;   // 日志文件的目录名
    std::string m_log_name;   // 日志文件的文件名
    std::ofstream m_log_filestream; // 日志文件流
    std::mutex m_mutex; // 用于互斥地增加记录数
    std::shared_ptr<BlockQueue<std::string>> m_log_queue;   // 存放日志的阻塞队列
    std::thread m_write_thread; // 写入线程
    size_t m_count; // 统计一天中的日志条目数
    size_t m_split_lines;   // 分割行数
    bool m_is_async;    // 是否异步写入
    int m_today;    // 当前日志文件记录的时间
    bool m_close_log;   // 是否关闭日志
};

#endif