#include <DedupServer.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h> 
#include <sys/epoll.h>
#include <errno.h>
#include <Util.h>
#include <ClientConn.h>
#include <iostream>
#include <Logger.h>

DedupServer::DedupServer(int port, size_t thread_num, size_t queue_size)
    :m_port(port), m_thread_num(thread_num), m_queue_size(queue_size) {
    // 创建线程池
    m_threadPool = new ThreadPool(m_thread_num, m_queue_size);
    // 创建事件集合
    m_events = new epoll_event[5];
    // 创建mysql连接池
    m_mysqlPool = std::make_shared<MysqlConnPool>("localhost", 3306, "bluesky", "alqoZM123_", "dedup_server", 8);

    m_all_chunks = std::make_shared<std::map<std::string, ChunkAddr_t>>();
}

DedupServer::~DedupServer() {
    delete m_threadPool;
    delete m_events;
}

void DedupServer::init() {
    // 创建监听socket
    m_listenfd = socket(PF_INET, SOCK_STREAM, 0);

    int reuse = 1;
    setsockopt(m_listenfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    sockaddr_in addr;
    bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(m_port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    bind(m_listenfd, (sockaddr*)&addr, sizeof(addr));

    listen(m_listenfd, 10);

    // epoll
    m_epollfd = epoll_create(1);
    epoll_event event;
    event.data.fd = m_listenfd;
    event.events = EPOLLIN; // 监听socket上的可读事件
    epoll_ctl(m_epollfd, EPOLL_CTL_ADD, m_listenfd, &event);

    // logger
    Logger::get_instance().init("logs/ServerLog", CLOSELOG, LOGLINES, 100);
}

void DedupServer::run() {
    LOG_INFO("server starts to run");
    sockaddr_in clientAddr;
    socklen_t addrLen;
    ThreadPool::Request req;
    req.func = stub;

    while(true) {
        int ret = epoll_wait(m_epollfd, m_events, 5, -1);
        if(ret == -1 && errno != EINTR) {
            Util::errExit("epoll_wait() failed");
        }
        for(int i = 0; i < ret; i++) {
            int eventfd = m_events[i].data.fd;
            if(eventfd == m_listenfd) {
                int sockfd = accept(m_listenfd, (sockaddr*)&clientAddr, &addrLen);
                ClientInfo_t client_info(inet_ntoa(clientAddr.sin_addr), clientAddr.sin_port);
                ClientConn* clientConn = new ClientConn(sockfd, client_info, this);
                m_clients.insert(std::make_pair(sockfd, clientConn));
                Util::addfd(m_epollfd, sockfd);
                req.arg = clientConn;
                m_threadPool->append(req);
            } else if(m_events[i].events & EPOLLRDHUP) {
                Util::delfd(m_epollfd, eventfd);
                delete m_clients[eventfd];
                m_clients.erase(eventfd);
            }
        }
    }
}

void* DedupServer::stub(void* arg) {
    ClientConn* conn = (ClientConn*)arg;
    conn->run();
    return arg;
}

bool DedupServer::isFileUnique(const int client_id, const std::string file_name) {
  MysqlConn conn(m_mysqlPool);
  char query[256];
  sprintf(query, "select * from FILES where ID = %d and FileName = '%s'", client_id, file_name.c_str());
  return !conn.hasResult(query);
}

bool DedupServer::isFileExist(const int client_id, const std::string file_name) {
  return !isFileUnique(client_id, file_name);
}

void DedupServer::addFile(const int client_id, const std::string file_name) {
    MysqlConn conn(m_mysqlPool);
    char query[256];
    sprintf(query, "insert into FILES(ID, FileName) values(%d,'%s')", client_id, file_name.c_str());
    conn.executeInsertion(query);
    LOG_INFO("[%d] add the file %s into the database", client_id, file_name.c_str());
}

void DedupServer::addChunk(std::pair<std::string, ChunkAddr_t> chunk) {
    std::lock_guard<std::mutex> lg(m_mutex);
    m_all_chunks->insert(chunk);
}

bool DedupServer::isChunkStored(std::string strChunkHash, ChunkAddr_t& chunkAddr) {
    std::lock_guard<std::mutex> lg(m_mutex);
    auto it = m_all_chunks->find(strChunkHash);
    if(it == m_all_chunks->end()) {
        return false;
    } else {    
        chunkAddr = it->second;
        return true;
    }
}