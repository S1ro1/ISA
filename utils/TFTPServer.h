//
// Created by Matej Sirovatka on 14.10.2023.
//

#ifndef ISA_PROJECT_TFTPSERVER_H
#define ISA_PROJECT_TFTPSERVER_H

#include <arpa/inet.h>
#include <fstream>
#include <iostream>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

#include "ArgParser.h"
#include "Connection.h"
#include "TFTPPacket.h"
#include "utils.h"

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


#endif//ISA_PROJECT_TFTPSERVER_H
