#include <iostream>
#include <DedupServer.h>
#include <Config.h>
#include <Logger.h>
#include <openssl/evp.h>
using namespace std;


int main(int argc, char* argv[]) {
    OPENSSL_init_crypto(0, nullptr);
    
    ServerConfig config;
    config.parse_args(argc, argv);

    DedupServer server(config.m_port, config.m_thread_num, config.m_queue_size);

    server.init();

    server.run();
}