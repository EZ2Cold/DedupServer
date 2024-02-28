#ifndef UTIL_H_
#define UTIL_H_ 
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <define.h>
#include <string.h>
#include <Config.h>
#include <openssl/evp.h>
#include <sys/epoll.h>
#include <time.h>

class Util {
public:
    // 输出错误信息并退出
    static void errExit(const char* msg) {
        printf("%s\n", msg);
        exit(EXIT_FAILURE);
    }

    // 向指定套接字发送数据
    static void send_data(int sockfd, const uint8_t* data, size_t dataSize) {
        size_t sentSize = 0, totalSentSize = 0;
        write(sockfd, &dataSize, sizeof(dataSize));

        while(totalSentSize < dataSize) {
            sentSize = write(sockfd, data + totalSentSize, dataSize - totalSentSize);
            totalSentSize += sentSize;
        }
    }

    // 从指定套接字接收数据
    static void receive_data(int sockfd, uint8_t* data, size_t& receivedSize) {
        size_t len, readSize;
        readSize = read(sockfd, &len, sizeof(len));

        receivedSize = 0;
        while(receivedSize < len) {
            readSize = read(sockfd, data + receivedSize, len - receivedSize);
            receivedSize += readSize;
        }
    }

    static std::string get_file_name(const std::string& filePath) {
        std::string fileName = filePath;
        int i;
        for (i = filePath.size(); i >= 0; --i) {
            if (filePath[i] == '/') {
            break;
            }
        }
        if (i >= 0) {
            fileName = filePath.substr(i + 1);
        }
        return fileName;
    }

    static void send_control_message(int sockfd,
                                    Order_t order,
                                    const std::string fileName = "",
                                    RepStatus_t status = NONE) {
        ControlMsg_t* msg;
        size_t msgSize = 0;
        switch (order)
        {
        case UPLOAD:
        case DOWNLOAD:
            msgSize = sizeof(ControlMsg_t)+fileName.size()+1;
            msg = (ControlMsg_t*)malloc(msgSize);
            memcpy(msg->data, fileName.c_str(), fileName.size());
            break;
        case UPLOADREP:
        case DOWNLOADREP:
            msgSize = sizeof(ControlMsg_t);
            msg = (ControlMsg_t*)malloc(msgSize);
            msg->status = status;
            break;
        case QUIT:
        case QUITREP:
            msgSize = sizeof(ControlMsg_t);
            msg = (ControlMsg_t*)malloc(msgSize);
            break;
        }
        msg->order = order;
        send_data(sockfd, (uint8_t*)msg, msgSize);
        free(msg);
    }

    static void receive_control_msg(int sockfd, 
                                    Order_t& order, 
                                    std::string& fileName,
                                    RepStatus_t& status) {
        ControlMsg_t* msg = (ControlMsg_t*)malloc(sizeof(ControlMsg_t) + 200);;
        size_t msgSize;
        receive_data(sockfd, (uint8_t*)msg, msgSize);
        order = msg->order;
        switch (order)
        {
        case UPLOAD:
        case DOWNLOAD:
            fileName = std::string((char*)msg->data);
            break;
        case UPLOADREP:
        case DOWNLOADREP:
            status = msg->status;
            break;
        case QUIT:
        case QUITREP:
            break;
        }
        free(msg);
    }

    static void generate_hash(const uint8_t *data, 
                                size_t dataSize, 
                                unsigned char*hash,
								unsigned int *hashSize) {
        switch (HASHTYPE) {
        case SHA1:
            if(!EVP_Digest(data, dataSize, hash, hashSize, EVP_sha1(), nullptr)) {
                errExit("hash failed");
            }
            break;
        case SHA256:
            if(!EVP_Digest(data, dataSize, hash, hashSize, EVP_sha256(), nullptr)) {
                errExit("hash failed");
            }
        break;
        case MD5:
            if(!EVP_Digest(data, dataSize, hash, hashSize, EVP_md5(), nullptr)) {
                errExit("hash failed");
            }
        break;
        }
    }

    static std::string convert_array_hash_to_string(uint8_t hash[32]) {
        char strHash[65];
        int i = 0;
        for(int j = 0; j < 32; j++) {
            uint8_t tmp = (hash[j] & 0xf0) >> 4;
            if(tmp <= 9) {
            strHash[i++] = tmp + '0';
            } else {
            strHash[i++] = tmp - 10 + 'a';
            }
            tmp = (hash[j] & 0x0f);
            if(tmp <= 9) {
            strHash[i++] = tmp + '0';
            } else {
            strHash[i++] = tmp - 10 + 'a';
            }
        }
        strHash[i] = '\0';
        // printf("enclave hash: %s\n", strHash);
        return std::string(strHash);
    }

    static void addfd(int epoll_fd, int sock_fd) {
        epoll_event event;
        event.data.fd = sock_fd;
        event.events = EPOLLRDHUP; // 监听socket上的可读事件
        epoll_ctl(epoll_fd, EPOLL_CTL_ADD, sock_fd, &event);
    }

    static void delfd(int epoll_fd, int sock_fd) {
        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, sock_fd, nullptr);
    }

    static tm* get_time() {
        time_t current_time = time(nullptr);
        return localtime(&current_time);
    }
};

#endif