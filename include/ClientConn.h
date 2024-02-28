#ifndef CLIENTCONN_H_
#define CLIENTCONN_H_
// #include <DedupServer.h>
#include <define.h>
#include <queue>
#include <BlockQueue.h>

class DedupServer;

class ClientConn {
public:
    int m_sockfd;
    DedupServer* m_server;
    DataMsg_t* m_inMsgBuffer, *m_outMsgBuffer;
    // for upload
    HashList_t* m_hashList;  // hashes in the last query
    std::queue<ChunkAddr_t> m_duplicatedChunkAddr;
    bool m_fileEnd; // whether the upload of file is end
    std::shared_ptr<BlockQueue<Container_t>> m_containerQueue;
    Container_t m_containerBuffer;
    RecipeEntryContainer_t m_recipeBuffer;
    std::shared_ptr<BlockQueue<RecipeEntryContainer_t>> m_recipeQueue;
    std::string m_recipeNameHash;   // hash of the recipe file
    // for download
    uint8_t* m_chunkAddrs;

public:
    ClientConn(int sockfd, ClientInfo_t client_info, DedupServer* server);
    ~ClientConn();
    void run();
private:
    ClientInfo_t m_client_info;
    void receive_file(std::string fileName);
    void send_file(std::string fileName);
    void write_container();
    void write_recipe();
    void save_container(Container_t& container);
    void reset_upload();
    void reset_download();
    void new_container();
    void new_recipe();
    void insert_container_queue();
    void insert_recipe_queue();
};

#endif