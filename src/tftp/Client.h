//
// Created by Matej Sirovatka on 09.10.2023.
//

#ifndef ISA_PROJECT_CLIENT_H
#define ISA_PROJECT_CLIENT_H

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <csignal>

#include "../utils/ArgParser.h"
#include "../utils/IInputWrapper.h"
#include "../utils/IOutputWrapper.h"
#include "../utils/Options.h"
#include "../utils/utils.h"
#include "Packet.h"
#include "common.h"

namespace TFTP {
  class Client {
    int mSocketFd;
    sockaddr_in mServerAddress;
    sockaddr_in mClientAddress;

    uint16_t mClientPort;

    State mState;
    std::string mTransmissionMode;

    int mBlockNumber;
    Mode mMode;
    std::string mSrcFilePath;
    std::string mDestFilePath;
    std::optional<ErrorPacket> mErrorPacket;

    std::unique_ptr<Packet> mLastPacket;

    Options::map_t mOptions;

    void sendPacket(const Packet &packet);

    std::unique_ptr<Packet> receivePacket();

  public:
    explicit Client(const ClientArgs &args, Options::map_t opts);

    ~Client() {
      close(mSocketFd);
    }

    void transmit();

    void requestRead();

    void requestWrite();

    std::unique_ptr<Packet> exchangePackets(const Packet &packet, bool send);
  };
}// namespace TFTP


#endif//ISA_PROJECT_CLIENT_H
