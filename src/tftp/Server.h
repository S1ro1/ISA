// Matej Sirovatka, xsirov00

#ifndef ISA_PROJECT_SERVER_H
#define ISA_PROJECT_SERVER_H

#include <csignal>
#include <thread>

#include "../utils/ArgParser.h"
#include "Connection.h"
#include "Packet.h"

namespace TFTP {
  /**
   * @brief Server class
   */
  class Server {
    int mMainSocketFd;
    sockaddr_in mServerAdress;
    std::string mRootDir;

    std::vector<std::thread> mThreads;
    std::vector<std::unique_ptr<Connection>> mConnections;

  public:
    /**
     * @brief Server constructor
     * @param args structure holding arguments passed to the program
     */
    explicit Server(const ServerArgs &args);

    ~Server();

    /**
     * @brief Starts listening for incoming connections
     */
    void listen();
  };
}// namespace TFTP


#endif//ISA_PROJECT_SERVER_H
