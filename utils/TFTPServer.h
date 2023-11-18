//
// Created by Matej Sirovatka on 14.10.2023.
//

#ifndef ISA_PROJECT_TFTPSERVER_H
#define ISA_PROJECT_TFTPSERVER_H

#include <arpa/inet.h>
#include <fstream>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>
#include <csignal>
#include <filesystem>

#include "ArgParser.h"
#include "Connection.h"
#include "TFTPPacket.h"

class TFTPServer {
  int main_socket_fd;
  sockaddr_in server_address;
  std::string root_dir;

  std::vector<std::thread> threads;
  std::vector<std::unique_ptr<Connection>> connections;

public:
  explicit TFTPServer(const ServerArgs &args);

  ~TFTPServer();

  void listen();
};


#endif//ISA_PROJECT_TFTPSERVER_H
