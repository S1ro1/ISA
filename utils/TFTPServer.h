//
// Created by Matej Sirovatka on 14.10.2023.
//

#ifndef ISA_PROJECT_TFTPSERVER_H
#define ISA_PROJECT_TFTPSERVER_H

#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string>
#include <fstream>
#include <iostream>
#include <thread>

#include "TFTPPacket.h"
#include "ArgParser.h"
#include "utils.h"
#include "Connection.h"

class TFTPServer {
    int main_socket_fd;
    sockaddr_in server_address;
    std::string root_dir;

    std::vector<std::jthread> threads;


public:
    explicit TFTPServer(const ServerArgs &args);

    ~TFTPServer() {
        close(main_socket_fd);
    }


    void listen();
};


#endif //ISA_PROJECT_TFTPSERVER_H
