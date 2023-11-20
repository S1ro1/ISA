//
// Created by Matej Sirovatka on 14.10.2023.
//

#ifndef ISA_PROJECT_SERVER_H
#define ISA_PROJECT_SERVER_H

#include <thread>
#include <csignal>

#include "../utils/ArgParser.h"
#include "Connection.h"
#include "Packet.h"

namespace TFTP {
  class Server {
    int main_socket_fd;
    sockaddr_in server_address;
    std::string root_dir;

    std::vector<std::thread> threads;
    std::vector<std::unique_ptr<Connection>> connections;

  public:
    explicit Server(const ServerArgs &args);

    ~Server();

    void listen();
  };
}// namespace TFTP


#endif//ISA_PROJECT_SERVER_H
