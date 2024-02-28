#include <Logger.h>
#include <string.h>
#include <Util.h>

// split_lines 日志文件行数超过split_lines, 就创建新的日志文件
// queue_size 异步写入 阻塞队列的长度
void Logger::init(const char* file_path, bool close_log, size_t split_lines, size_t queue_size) {
    // 创建日志文件 在文件名前面加上年月日
    char full_log_path[256];
    memset(full_log_path, 0, 256);
    tm* cur_time = Util::get_time();

    const char* p = strrchr(file_path, '/');
    if(p == nullptr) {  // 不包含目录
        m_log_name = file_path;
        sprintf(full_log_path, "%d_%02d_%02d_%s", cur_time->tm_year+1900, cur_time->tm_mon+1, cur_time->tm_mday, file_path);
    } else {    // 包含目录
        m_log_name = std::string(p+1);
        m_dir_name = std::string(file_path, p-file_path+1);
        sprintf(full_log_path, "%s%d_%02d_%02d_%s", m_dir_name.c_str(), cur_time->tm_year+1900, cur_time->tm_mon+1, cur_time->tm_mday, m_log_name.c_str());
    }
    m_log_filestream.open(full_log_path, std::fstream::out);

    m_today = cur_time->tm_mday;
    m_count= 0;
    m_is_async = false;
    m_split_lines = split_lines;
    m_close_log = close_log;

    // 异步写入 创建工作线程
    if(queue_size > 0) {
        m_log_queue = std::make_shared<BlockQueue<std::string>>(queue_size);
        m_is_async = true;
        m_write_thread = std::thread(&Logger::async_write_log, this);
    }
}

Logger::~Logger() {
    if(m_is_async) {
        m_log_queue->set_done();
        m_write_thread.join();
    }
    m_log_filestream.close();
}

void Logger::write_log(Level level, const char* fmt, ...) {
    if(m_close_log) return;
    tm*  cur_time = Util::get_time();
    // 构造日志记录
    char one_log[LOGBUFSIZE];
    memset(one_log, 0, LOGBUFSIZE);
    std::string tmp;
    switch(level) {
        case DEBUG:
            tmp = "[debug]:";
            break;
        case INFO:
            tmp = "[info]:";
            break;
        case WARN:
            tmp = "[warn]:";
            break;
        case ERROR:
            tmp = "[error]:";
            break;
        default:
            tmp = "[info]:";
            break;
    }
    int n = sprintf(one_log, "%d-%02d-%02d %02d-%02d-%02d %s", cur_time->tm_year+1900, cur_time->tm_mon+1, cur_time->tm_mday, cur_time->tm_hour, cur_time->tm_min,cur_time->tm_sec, tmp.c_str());

    va_list valist;
    va_start(valist, fmt);
    int m = vsprintf(one_log+n, fmt, valist);
    va_end(valist);
    one_log[m+n] = '\n';
    one_log[m+n+1] = '\0';
    char new_log_path[256];
    bzero(new_log_path, 256);
    std::string str_log(one_log);
    
    m_mutex.lock();
    m_count++;
    if(cur_time->tm_mday != m_today || m_count % m_split_lines == 0) {
        // 创建新日志文件 新的一天 or 行数超过m_split_lines
        if(cur_time->tm_mday != m_today) {
            sprintf(new_log_path, "%s%d_%02d_%02d_%s", m_dir_name.c_str(), cur_time->tm_year+1900, cur_time->tm_mon+1, cur_time->tm_mday, m_log_name.c_str());
            m_count = 0;
            m_today = cur_time->tm_mday;
        } else {
            sprintf(new_log_path, "%s%d_%02d_%02d_%s.%lld", m_dir_name.c_str(), cur_time->tm_year+1900, cur_time->tm_mon+1, cur_time->tm_mday, m_log_name.c_str(), m_count/m_split_lines);
        }   
        if(m_is_async) {
            // 异步写入要等待队列中的日志全部写入完成
            while(!m_log_queue->empty()) ;
        }
        m_log_filestream.close();
        m_log_filestream.open(new_log_path, std::fstream::out);
    }
    m_mutex.unlock();
    if(m_is_async) {
        m_log_queue->push(str_log);
    } else {
        m_mutex.lock();
        m_log_filestream.write(str_log.c_str(), str_log.size());
        flush();
        m_mutex.unlock();
    }
    
}

void Logger::async_write_log() {
    std::string one_log;
    while(true) {
        if(!m_log_queue->pop(one_log))  return;
        m_log_filestream.write(one_log.c_str(), one_log.size());
        flush();
    }
}

void Logger::flush() {
    m_log_filestream.flush();
}