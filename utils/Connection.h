//
// Created by Matej Sirovatka on 14.10.2023.
//

#ifndef ISA_PROJECT_CONNECTION_H
#define ISA_PROJECT_CONNECTION_H

#include <arpa/inet.h>
#include <fstream>
#include <iostream>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>
#include <utility>

#include "ArgParser.h"
#include "TFTPPacket.h"
#include "utils.h"


class Connection {
  int socket_fd;
  sockaddr_in connection_address;
  sockaddr_in client_address;
  uint16_t connection_port;

  TFTPState state;
  Mode mode;
  uint16_t blkNumber = 0;

  std::string file_path;

  OptionsMap opts;

  void sendPacket(const TFTPPacket &packet);

  std::unique_ptr<TFTPPacket> receivePacket();

public:
  Connection(std::string file_path, OptionsMap options, sockaddr_in client_address);

  void serveDownload();

  void serveUpload();

  ~Connection() {
    LOG("Closing connection...");
    close(socket_fd);
  }
};


#endif//ISA_PROJECT_CONNECTION_H
