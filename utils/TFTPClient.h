//
// Created by Matej Sirovatka on 09.10.2023.
//

#ifndef ISA_PROJECT_TFTPCLIENT_H
#define ISA_PROJECT_TFTPCLIENT_H

#include <arpa/inet.h>
#include <cstring>
#include <fstream>
#include <iostream>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <unistd.h>

#include "ArgParser.h"
#include "TFTPPacket.h"
#include "utils.h"


class TFTPClient {
  int socket_fd;
  sockaddr_in server_address;
  sockaddr_in client_address;

  uint16_t client_port;

  TFTPState state;
  std::string transmissionMode = "octet";

  Mode mode;
  std::string src_file_path;
  std::string dst_file_path;

  OptionsMap opts;

  void sendPacket(const TFTPPacket &packet);

  std::unique_ptr<TFTPPacket> receivePacket();

public:
  explicit TFTPClient(const ClientArgs &args);

  ~TFTPClient() {
    close(socket_fd);
  }

  void transmit();

  void requestRead();

  void requestWrite();

  void handleDataPacket(std::ofstream &outputFile, DataPacket *data_packet);
};


#endif//ISA_PROJECT_TFTPCLIENT_H
