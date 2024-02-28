#include <DedupClient.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <Util.h>
#include <iostream>
#include <fstream>
#include <Config.h>
#include <math.h>
#include <Logger.h>
#include <thread>

DedupClient::DedupClient(int client_id, const char* server_addr, int port, char op, std::string file_path)
    : m_client_id(client_id), m_server_addr(server_addr), m_port(port), m_op(op), m_file_path(file_path) {
      m_readBufferQueue = std::make_shared<BlockQueue<ReadBuffer_t>>(1000);
      m_outMsgBuffer = (DataMsg_t*)malloc(sizeof(DataMsg_t) + (CHUNKSIZE+HASHSIZE)*BATCHSIZE);
      m_inMsgBuffer = (DataMsg_t*)malloc(sizeof(DataMsg_t) + CHUNKSIZE*BATCHSIZE);
}

DedupClient::~DedupClient() {
  free(m_outMsgBuffer);
  free(m_inMsgBuffer);
}

bool DedupClient::connect_server() {
    m_sockfd = socket(PF_INET, SOCK_STREAM, 0);
    
    sockaddr_in addr;
    bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(m_port);
    addr.sin_addr.s_addr = inet_addr(m_server_addr);

    if(connect(m_sockfd, (sockaddr*)&addr, sizeof(addr)) == -1) return false;

    return true;

}

void DedupClient::run() {
    LOG_INFO("client starts to run");
    DataMsg_t* msg = (DataMsg_t*)malloc(sizeof(DataMsg_t)+sizeof(LoginInfo_t));
    msg->header.dataMsgType = LOGIN;
    msg->header.size = sizeof(DataMsg_t)+sizeof(LoginInfo_t);
    ((LoginInfo_t*)msg->data)->client_id = m_client_id;
    Util::send_data(m_sockfd, (uint8_t*)msg, msg->header.size);
    free(msg);
    
    switch (m_op)
    {
    case 'u':
        upload_file();
        break;
    case 'd':
        download_file();
        break;
    default:
        break;
    }
    LOG_INFO("send quit message");
    Util::send_control_message(m_sockfd, QUIT);
    Order_t order;
    std::string tmpFileName;
    RepStatus_t status;
    Util::receive_control_msg(m_sockfd, order, tmpFileName, status);
    if(order == QUITREP) {
      close(m_sockfd);
      LOG_INFO("receive quit message response");
    } 
}

void DedupClient::upload_file() {

  std::string fileName = Util::get_file_name(m_file_path);

  Util::send_control_message(m_sockfd, UPLOAD, fileName);

  Order_t order;
  std::string tmpFileName;
  RepStatus_t status;

  Util::receive_control_msg(m_sockfd, order, tmpFileName, status);

  if(order == UPLOADREP) {
    if(status == DUPLICATE) {
      return;
    }
  } else {
    return;
  }

  std::thread read_data_thread(&DedupClient::read_data, this);
  read_data_thread.detach();

  size_t totalChunkNum = 0, totalUniqueNum = 0, totalFileSize = 0, currentBatchNum = 0;
  ReadBuffer_t readBuffer;
  m_outMsgBuffer->header.dataMsgType = UPLOADBODY;
  while (true) {
    size_t chunksOccupied = 0;
    if(totalChunkNum != 0) {
      // receive response and check which chunk is unique
      size_t receivedSize;
      Util::receive_data(m_sockfd, (uint8_t*)m_inMsgBuffer, receivedSize);

      uint32_t currentUniqueNum = 0;
      for(int i = 0; i < currentBatchNum; i++) {
        // 1:unique need to upload
        if(m_inMsgBuffer->data[i] == 1) {
          uint32_t start = i * CHUNKSIZE, end = std::min((i+1)*CHUNKSIZE, readBuffer.dataSize);
          memcpy(m_outMsgBuffer->data + chunksOccupied, readBuffer.dataBuffer+start, end-start);
          chunksOccupied += end - start;
          ++currentUniqueNum;
        } 
      }
      m_outMsgBuffer->header.chunkNum = currentUniqueNum;
      totalUniqueNum += currentUniqueNum;

      free(readBuffer.dataBuffer);
      free(readBuffer.hashBuffer);
    }

    if(!m_readBufferQueue->pop(readBuffer)) {
      readBuffer.chunkNum = 0;
      readBuffer.dataSize = 0;
    }

    // fill hash of new chunks
    totalFileSize += readBuffer.dataSize;
    currentBatchNum = readBuffer.chunkNum;
    m_outMsgBuffer->header.queryNum = currentBatchNum;
    m_outMsgBuffer->header.start = chunksOccupied;
    memcpy(m_outMsgBuffer->data + chunksOccupied, readBuffer.hashBuffer, readBuffer.chunkNum * HASHSIZE);


    // send the outMsgBuffer
    m_outMsgBuffer->header.size = sizeof(DataMsg_t) + chunksOccupied + currentBatchNum * HASHSIZE;
    Util::send_data(m_sockfd, (uint8_t*)m_outMsgBuffer, m_outMsgBuffer->header.size);

    totalChunkNum += currentBatchNum;

    if(currentBatchNum == 0)  break;
  }
  m_outMsgBuffer->header.dataMsgType = FILEEND;
  m_outMsgBuffer->header.size = sizeof(DataMsg_t);
  Util::send_data(m_sockfd, (uint8_t*)m_outMsgBuffer, m_outMsgBuffer->header.size);

  // uint32_t receivedSize;
  // sslConnection_->ReceiveData((uint8_t*)inMsgBuffer, receivedSize);
}

void DedupClient::download_file() {
  std::fstream fileStream;
  std::string fileName = Util::get_file_name(m_file_path);
  fileStream.open(m_file_path, std::ios::out);

  Util::send_control_message(m_sockfd, DOWNLOAD, fileName);

  Order_t order;
  std::string tmpFileName;
  RepStatus_t status;

  Util::receive_control_msg(m_sockfd, order, tmpFileName, status);

  if(order == DOWNLOADREP) {
    if(status == NOTEXIST) {
      return;
    }
  } else {
    return;
  }

  uint32_t totalChunkNum = 0, totalFileSize = 0;
  size_t receivedSize;
  while(true) {
    Util::receive_data(m_sockfd, (uint8_t*)m_inMsgBuffer, receivedSize);
    if (m_inMsgBuffer->header.dataMsgType == FILEEND) {
      break;
    }

    size_t dataSize = m_inMsgBuffer->header.size - sizeof(DataMsg_t);
    fileStream.write(reinterpret_cast<char*>(m_inMsgBuffer->data), dataSize);
    totalChunkNum += m_inMsgBuffer->header.chunkNum;
    totalFileSize += dataSize;
  }
  fileStream.close();
}

void DedupClient::read_data() {
  ReadBuffer_t readBuffer;

  std::fstream fileStream;
  fileStream.open(m_file_path, std::ios::in);
  if (!fileStream.is_open()) {
    Util::errExit("cannot open the file");
  }
  
  size_t data_len = CHUNKSIZE*BATCHSIZE;
  size_t hash_len = HASHSIZE*BATCHSIZE;
  while(true) {
    readBuffer.dataBuffer = (uint8_t*)malloc(data_len);
    readBuffer.hashBuffer = (uint8_t*)malloc(hash_len);
    // read new chunks
    fileStream.read(reinterpret_cast<char*>(readBuffer.dataBuffer), data_len);
    size_t len = fileStream.gcount();
    if(len <= 0)  break;
    readBuffer.chunkNum = ceil(len*1.0/CHUNKSIZE);
    readBuffer.dataSize =len;

    unsigned int hashSize;
    for(uint32_t i = 0; i < readBuffer.chunkNum; i++) {
      uint32_t start = i * CHUNKSIZE, end = std::min((i+1)*CHUNKSIZE, len);
      Util::generate_hash(readBuffer.dataBuffer+start, end-start, readBuffer.hashBuffer + i*HASHSIZE, &hashSize);
    }

    m_readBufferQueue->push(readBuffer);
  }
  m_readBufferQueue->set_done();
  fileStream.close();
}