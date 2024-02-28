#include <Config.h>
#include <argparse.hpp>
#include <Util.h>

void ServerConfig::parse_args(int argc, char* argv[]) {

}

void ClientConfig::parse_args(int argc, char* argv[]) {
    argparse::ArgumentParser program("client", "1.0");
    program.add_argument("-i", "--identity")
      .help("client id")
      .required();
    program.add_argument("-o", "--operation")
      .help("specifies operations to be done\n\t\t\t u:upload the file\n\t\t\t d:download the file")
      .required();
    program.add_argument("-f", "--file")
      .help("(path of file to be uploaded) or (path of file to be saved)")
      .required();

    try {
        program.parse_args(argc, argv);
    } catch (const std::runtime_error& err) {
        Util::errExit("parse arguments failed.");
    }

    m_op = program.get<std::string>("--operation")[0];
    m_file_path = program.get<std::string>("--file");
    m_client_id = std::stoi(program.get<std::string>("--identity"));
}

ClientConfig::~ClientConfig() {
}