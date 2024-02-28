#include <ClientConn.h>
#include <Util.h>
#include <stdint.h>
#include <iostream>
#include <Config.h>
#include <fstream>
#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <DedupServer.h>
#include <Logger.h>
#include <string>
#include <thread>

ClientConn::ClientConn(int sockfd, ClientInfo_t client_info, DedupServer* server)
    : m_sockfd(sockfd), m_client_info(client_info), m_server(server) {
  
  m_inMsgBuffer = (DataMsg_t*)malloc(sizeof(DataMsg_t) + (CHUNKSIZE+HASHSIZE) * BATCHSIZE);
  m_outMsgBuffer = (DataMsg_t*)malloc(sizeof(DataMsg_t) + CHUNKSIZE*BATCHSIZE);
  m_hashList = (HashList_t*)malloc(sizeof(HashList_t)+HASHSIZE*BATCHSIZE);
  m_hashList->num = 0;
  m_containerQueue = std::make_shared<BlockQueue<Container_t>>(1000);
  m_recipeQueue = std::make_shared<BlockQueue<RecipeEntryContainer_t>>(1000);
  m_chunkAddrs = (uint8_t*)malloc(sizeof(ChunkAddr_t)*BATCHSIZE);
  new_container();
  new_recipe();
}

ClientConn::~ClientConn() {
  free(m_inMsgBuffer);
  free(m_outMsgBuffer);
  free(m_hashList);
  free(m_chunkAddrs);
}

void ClientConn::run() {
  size_t rz;
  DataMsg_t* msg = (DataMsg_t*)malloc(sizeof(DataMsg_t)+sizeof(LoginInfo_t));
  Util::receive_data(m_sockfd, (uint8_t*)msg, rz);
  m_client_info.client_id = ((LoginInfo_t*)msg->data)->client_id;
  free(msg);
  LOG_INFO("[%d] thread %lu is serving the client with address %s:%d", m_client_info.client_id, std::this_thread::get_id(), m_client_info.ip.c_str(), m_client_info.port);
  bool run = true;
  unsigned int hashSize;
  unsigned char fileNameHash[32];
  std::string strFileNameHash;
  while(run) {
    Order_t order;
    std::string fileName;
    RepStatus_t status;

    Util::receive_control_msg(m_sockfd, order, fileName, status);

    switch(order) {
      case UPLOAD:{
        receive_file(fileName);
        break;
      }
      case DOWNLOAD:{
        send_file(fileName);
        break;
      }
      case QUIT:{
        LOG_INFO("[%d] receive quit message", m_client_info.client_id);
        Util::send_control_message(m_sockfd, QUITREP);
        close(m_sockfd);
        run = false;
        break;
      }
    }
  }
}

void ClientConn::receive_file(std::string fileName) {
  LOG_INFO("[%d] receive upload request", m_client_info.client_id);
  // statistic variables
  size_t received_file_size = 0, unique_chunk_num = 0, duplicated_chunk_num = 0;

  unsigned int hashSize;
  unsigned char recipeNameHash[32];
  // generate hash of client_id+fileName
  std::string recipeName = std::to_string(m_client_info.client_id) + fileName;
  Util::generate_hash((uint8_t*)recipeName.c_str(), recipeName.size(), recipeNameHash, &hashSize);
  
  m_recipeNameHash = Util::convert_array_hash_to_string(recipeNameHash);
  
  // check uniqueness of file
  if(!m_server->isFileUnique(m_client_info.client_id, fileName)) {
    LOG_WARN("[%d] file has been uploaded before", m_client_info.client_id);
    Util::send_control_message(m_sockfd, UPLOADREP, "", DUPLICATE);
    return;
  } else {
    LOG_INFO("[%d] file does not exist in the database", m_client_info.client_id);
    Util::send_control_message(m_sockfd, UPLOADREP, "", UNIQUE);
    m_server->addFile(m_client_info.client_id, fileName);
  }

  // new thread to write recipes and containers
  std::thread dataWriter(&ClientConn::write_container, this), recipeWriter(&ClientConn::write_recipe, this);

  size_t receivedSize;
  m_outMsgBuffer->header.dataMsgType = UPLOADBODYREP;
  while(true) {
    Util::receive_data(m_sockfd, (uint8_t*)m_inMsgBuffer, receivedSize);
    // if end of file, return
    if (m_inMsgBuffer->header.dataMsgType == FILEEND) {
      // save the last container and recipes
      if(m_containerBuffer.occupied != 0) {
        insert_container_queue();
      }
      insert_recipe_queue();

      m_outMsgBuffer->header.dataMsgType = FILEEND;

    } else {
      // save to the container
      size_t currentUniqueNum = 0;
      ChunkAddr_t chunkAddr;
      for(int i = 0; i < m_hashList->num; i++) {
        std::string strChunkHash = Util::convert_array_hash_to_string(m_hashList->data + i * HASHSIZE);
        if(m_outMsgBuffer->data[i] == 1) {
          uint32_t start = currentUniqueNum * CHUNKSIZE, end = std::min((currentUniqueNum+1)*CHUNKSIZE, m_inMsgBuffer->header.start);
          uint32_t chunkSize = end - start;
          received_file_size += chunkSize;
          // write to the container
          if(m_containerBuffer.capacity < chunkSize) {
            // write container
            insert_container_queue();
            new_container();
          }
          memcpy(m_containerBuffer.data+m_containerBuffer.occupied, m_inMsgBuffer->data+start, chunkSize);
          m_containerBuffer.occupied += chunkSize;
          m_containerBuffer.capacity -= chunkSize;

          chunkAddr = ChunkAddr_t(m_containerBuffer.containerID, m_containerBuffer.occupied - chunkSize, chunkSize);


          // insert this chunk to the hashmap storedChunks
          m_server->addChunk(std::make_pair(strChunkHash, chunkAddr));
          ++currentUniqueNum;
        } else if (m_outMsgBuffer->data[i] == 0) {
          // find the chunkAddr in the hashmap storedChunks and write the recipe
          chunkAddr = m_duplicatedChunkAddr.front();
          m_duplicatedChunkAddr.pop();
        }
        // write the recipe entry
        if(m_recipeBuffer.num >= BATCHSIZE) {
          insert_recipe_queue();
          new_recipe();
        }
        memcpy(m_recipeBuffer.data + m_recipeBuffer.num * sizeof(ChunkAddr_t), &chunkAddr, sizeof(ChunkAddr_t));
        ++m_recipeBuffer.num;
      }
      unique_chunk_num += currentUniqueNum;
      // process new query
      m_hashList->num = m_inMsgBuffer->header.queryNum;
      m_outMsgBuffer->header.queryNum = m_inMsgBuffer->header.queryNum;
      if(m_inMsgBuffer->header.queryNum != 0) {
        // copy the quried hash to the checkList
        memcpy(m_hashList->data, m_inMsgBuffer->data + m_inMsgBuffer->header.start, HASHSIZE * m_inMsgBuffer->header.queryNum);

        for(int i = 0; i < m_inMsgBuffer->header.queryNum; i++) {
          size_t start = m_inMsgBuffer->header.start + i * HASHSIZE;
          std::string strChunkHash = Util::convert_array_hash_to_string(m_inMsgBuffer->data+start);
          
          if(m_server->isChunkStored(strChunkHash, chunkAddr)) {
            m_outMsgBuffer->data[i] = 0;
            m_duplicatedChunkAddr.push(chunkAddr);
          } else {
            m_outMsgBuffer->data[i] = 1;
          }
        }
      }
      duplicated_chunk_num += m_duplicatedChunkAddr.size();
      m_outMsgBuffer->header.size = sizeof(DataMsg_t) + m_outMsgBuffer->header.queryNum;
    }  
    if(m_outMsgBuffer->header.dataMsgType == UPLOADBODYREP) {
      if(m_outMsgBuffer->header.queryNum != 0) {
        Util::send_data(m_sockfd, (uint8_t*)m_outMsgBuffer, m_outMsgBuffer->header.size);
      }
    } else if(m_outMsgBuffer->header.dataMsgType == FILEEND) {
      m_containerQueue->set_done();
      m_recipeQueue->set_done();
      break;
    }
  }

  recipeWriter.join();
  dataWriter.join();

  reset_upload();
  LOG_INFO("[%d] upload finished! receive %lu bytes, %lu unique chunks, %lu duplicated chunks",m_client_info.client_id, received_file_size, unique_chunk_num, duplicated_chunk_num);
}

void ClientConn::reset_upload() {
  m_hashList->num = 0;
  m_containerQueue->undone();
  m_recipeQueue->undone();
  m_recipeNameHash = "";
  new_container();
  new_recipe();
}

void ClientConn::new_container() {
  m_containerBuffer.capacity = CONTAINERSIZE;
  m_containerBuffer.occupied = 0;
  // generate new containerID
  std::string containerID = boost::uuids::to_string(boost::uuids::random_generator()());
  memcpy(m_containerBuffer.containerID, containerID.c_str(), containerID.size()+1);
  // printf("new containerID %s\n", container->containerID);
  m_containerBuffer.data = (uint8_t*)malloc(CONTAINERSIZE);
}

void ClientConn::new_recipe() {
  m_recipeBuffer.num = 0;
  m_recipeBuffer.data = (uint8_t*)malloc(BATCHSIZE * sizeof(ChunkAddr_t));
}

void ClientConn::insert_container_queue() {
  m_containerQueue->push(m_containerBuffer);
}

void ClientConn::insert_recipe_queue() {
  m_recipeQueue->push(m_recipeBuffer);
}

void ClientConn::write_container() {
    // printf("DataWriter start working.\n");
    Container_t tmpContainer;
    while(m_containerQueue->pop(tmpContainer)) {
      save_container(tmpContainer);
      // release space occupied by the container
      free(tmpContainer.data);
    }
}

void ClientConn::write_recipe() {
    // open recipe file
  std::fstream recipeFile("recipes/" + m_recipeNameHash, std::fstream::out);
  uint32_t totalChunkNum = 0;
  recipeFile.write((char*)&totalChunkNum, sizeof(totalChunkNum));

  // printf("start writing recipes.\n");
  RecipeEntryContainer_t tmpRecipeEntryContainer;
  while(m_recipeQueue->pop(tmpRecipeEntryContainer)) {
      recipeFile.write((char*)tmpRecipeEntryContainer.data, tmpRecipeEntryContainer.num * sizeof(ChunkAddr_t));
      totalChunkNum += tmpRecipeEntryContainer.num;
      free(tmpRecipeEntryContainer.data);
  }
  // rewrite the totalChunkNum
  recipeFile.close();
  recipeFile.open("recipes/" + m_recipeNameHash, std::fstream::in | std::fstream::out);
  recipeFile.write((char*)&totalChunkNum, sizeof(totalChunkNum));
  recipeFile.close();
}

void ClientConn::save_container(Container_t& container) {
  // printf("containerID:%s\n", container.containerID);
  std::string filePath("containers/");
  filePath += container.containerID;
  std::fstream fileStream(filePath, std::fstream::out);
  if (!fileStream) {
    LOG_ERROR("open container file failed.\n");
  }
  fileStream.write((char*)container.data, container.occupied);
  fileStream.close();
}



void ClientConn::send_file(std::string fileName) {
  LOG_INFO("[%d] receive download request", m_client_info.client_id);
  unsigned int hashSize;
  unsigned char recipeNameHash[32];
  // generate hash of client_id+fileName
  std::string recipeName = std::to_string(m_client_info.client_id) + fileName;
  Util::generate_hash((uint8_t*)recipeName.c_str(), recipeName.size(), recipeNameHash, &hashSize);
  
  m_recipeNameHash = Util::convert_array_hash_to_string(recipeNameHash);
  
  // check existness of file
  if(!m_server->isFileExist(m_client_info.client_id, fileName)) {
    LOG_WARN("[%d] file does not exist", m_client_info.client_id);
    Util::send_control_message(m_sockfd, DOWNLOADREP, "", NOTEXIST);
    return;
  } else {
    LOG_INFO("[%d] file exists", m_client_info.client_id);
    Util::send_control_message(m_sockfd, DOWNLOADREP, "", EXIST);
  }
  std::string strFilePath = "recipes/" + m_recipeNameHash;
  std::fstream recipe(strFilePath, std::fstream::in);

  uint32_t totalChunkNum = 0;
  recipe.read(reinterpret_cast<char*>(&totalChunkNum), sizeof(uint32_t));
  m_outMsgBuffer->header.dataMsgType = DOWNLOADBODY;
  size_t total_sent_file_size = 0;
  while(true) {
    size_t dataOffset = 0;
    recipe.read(reinterpret_cast<char*>(m_chunkAddrs), sizeof(ChunkAddr_t)*BATCHSIZE);
    auto len = recipe.gcount();
    if(len <= 0)    break;
    size_t currentChunkNum = len/sizeof(ChunkAddr_t);
    for(int i = 0; i < currentChunkNum; i++) {
        ChunkAddr_t* chunkAddr = reinterpret_cast<ChunkAddr_t*>(m_chunkAddrs+i*sizeof(ChunkAddr_t));
        std::string containerPath("containers/");
        containerPath += std::string(chunkAddr->containerID);
        std::ifstream container(containerPath, std::ifstream::in);
        container.seekg(chunkAddr->offset);
        container.read(reinterpret_cast<char*>(m_outMsgBuffer->data+dataOffset), chunkAddr->size);
        dataOffset += chunkAddr->size;
        container.close();
    }
    m_outMsgBuffer->header.chunkNum = currentChunkNum;
    m_outMsgBuffer->header.size = sizeof(DataMsg_t) + dataOffset;
    Util::send_data(m_sockfd, (uint8_t*)m_outMsgBuffer, m_outMsgBuffer->header.size);
    total_sent_file_size += dataOffset;
  }
  m_outMsgBuffer->header.dataMsgType = FILEEND;
  Util::send_data(m_sockfd, (uint8_t*)m_outMsgBuffer, sizeof(DataMsg_t));
  recipe.close();
  reset_download();
  LOG_INFO("[%d] download finished. sent %lu bytes, %lu chunks", m_client_info.client_id, total_sent_file_size, totalChunkNum);
}

void ClientConn::reset_download() {
  m_recipeNameHash = "";
}