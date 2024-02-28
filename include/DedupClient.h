#ifndef DEDUPCLIENT_H_
#define DEDUPCLIENT_H_
#include <string>
#include <define.h>
#include <BlockQueue.h>

class DedupClient
{
public:
    const char* m_server_addr;
    int m_port;
    int m_sockfd;
    char m_op;
    std::string m_file_path;
    int m_client_id;
private:
    std::shared_ptr<BlockQueue<ReadBuffer_t>> m_readBufferQueue;
    DataMsg_t* m_outMsgBuffer, *m_inMsgBuffer;
public:
    DedupClient(int clinet_id, const char* server_addr, int port, char op, std::string file_path);
    ~DedupClient();
    void run();
    bool connect_server();
private:
    void upload_file();
    void download_file();
    void read_data();
};

#endif