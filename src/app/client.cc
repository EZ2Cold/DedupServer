#include <iostream>
#include <DedupClient.h>
#include <Config.h>
#include <Util.h>
#include <Logger.h>


int main(int argc, char* argv[]) {
    ClientConfig config;

    OPENSSL_init_crypto(0, nullptr);
    
    config.parse_args(argc, argv);

    DedupClient client(config.m_client_id, config.m_server_addr, config.m_port, config.m_op, config.m_file_path);

    Logger::get_instance().init("logs/ClientLog", CLOSELOG, LOGLINES, 100);

    if(!client.connect_server())    Util::errExit("connect to server failed");
     
    client.run();

    return 0;
}