#ifndef DEFINE_H_
#define DEFINE_H_
#include <string.h>
#include <stdint.h>
#include <string>

enum Order_t {
  UPLOAD = 0,
  UPLOADREP,
  DOWNLOAD,
  DOWNLOADREP,
  QUIT,
  QUITREP
};

enum RepStatus_t {
  NONE = 0,
  DUPLICATE,
  UNIQUE,
  EXIST,
  NOTEXIST,
};

typedef struct {
  Order_t order;
  RepStatus_t status;
  uint8_t data[];
}ControlMsg_t;

typedef struct {
  uint8_t* dataBuffer;
  uint8_t* hashBuffer;
  size_t chunkNum;
  size_t dataSize;
}ReadBuffer_t;

enum HashType {
  SHA256 = 0,
  MD5 = 1,
  SHA1 = 2
};

enum DataMsgType {
  UPLOADBODY,
  UPLOADBODYREP,
  DOWNLOADBODY,
  FILEEND,
  LOGIN,
};

typedef struct {
  DataMsgType dataMsgType;
  size_t start; // start position of chunk hashs
  size_t queryNum; // number of chunk hash in the later part of data
  size_t chunkNum; // number of chunk
  size_t size; // size of header and data
}DataMsgHeader_t;

typedef struct {
  DataMsgHeader_t header;
  uint8_t data[];
}DataMsg_t;

struct ChunkAddr_t{
  // boost::uuids::uuid containerID;
  char containerID[40];
  size_t offset;
  size_t size;
  // ChunkAddr_t(boost::uuids::uuid containerID_, uint32_t offset_, uint32_t size_):
  //                   containerID(containerID_), offset(offset_), size(size_) {}
  ChunkAddr_t() {};
  ChunkAddr_t(const char* containerID_, size_t offset_, size_t size_):
              offset(offset_), size(size_) {
    memcpy(containerID, containerID_, strlen(containerID_)+1);
  }
  void operator=(const ChunkAddr_t& chunkAddr) {
    memcpy(containerID, chunkAddr.containerID, sizeof(containerID));
    offset = chunkAddr.offset;
    size = chunkAddr.size;
  }
};

typedef struct {
  size_t num;
  uint8_t data[];
}HashList_t;

typedef struct {
  // boost::uuids::uuid containerID;
  char containerID[40];
  unsigned int occupied;
  unsigned int capacity;
  uint8_t* data;
}Container_t;


typedef struct {
  size_t num; // number of entries
  uint8_t* data;
}RecipeEntryContainer_t;

typedef struct ClientInfo_t {
  int client_id;
  std::string ip;
  uint16_t port;
  ClientInfo_t(const char* ip_, uint16_t port_)
    :ip(ip_), port(port_) {}
}ClientInfo_t;

typedef struct {
  int client_id;
}LoginInfo_t;
#endif