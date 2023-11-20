//
// Created by Matej Sirovatka on 14.10.2023.
//

#ifndef ISA_PROJECT_SERVER_H
#define ISA_PROJECT_SERVER_H

#include <csignal>
#include <thread>

#include "../utils/ArgParser.h"
#include "Connection.h"
#include "Packet.h"

namespace TFTP {
  class Server {
    int mMainSocketFd;
    sockaddr_in mServerAdress;
    std::string mRootDir;

    std::vector<std::thread> mThreads;
    std::vector<std::unique_ptr<Connection>> mConnections;

  public:
    explicit Server(const ServerArgs &args);

    ~Server();

    void listen();
  };
}// namespace TFTP


#endif//ISA_PROJECT_SERVER_H
