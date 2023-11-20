//
// Created by Matej Sirovatka on 14.10.2023.
//

#ifndef ISA_PROJECT_CONNECTION_H
#define ISA_PROJECT_CONNECTION_H

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <csignal>
#include <filesystem>

#include "../utils/ArgParser.h"
#include "../utils/IInputWrapper.h"
#include "../utils/IOutputWrapper.h"
#include "../utils/Options.h"
#include "../utils/utils.h"
#include "Packet.h"
#include "common.h"


namespace TFTP {
  class Connection {
    int mSocketFd;
    uint16_t mConnectionPort;
    sockaddr_in mConnectionAddr;

    sockaddr_in mClientAddr;

    State mState;
    std::string mTransmissionMode;
    uint16_t mBlockNumber;

    std::optional<ErrorPacket> mErrorPacket;
    std::unique_ptr<Packet> mLastPacket;

    std::string mFilePath;
    Options::map_t mOptions;

    void sendPacket(const Packet &packet);

    [[nodiscard]] std::unique_ptr<Packet> receivePacket() const;

    std::unique_ptr<Packet> sendAndReceive(const Packet &packetToSend, bool send);

    template<typename T>
    [[nodiscard]] std::unique_ptr<T> expectPacketType(std::unique_ptr<Packet> packet) {
      if (packet == nullptr) {
        return nullptr;
      }

      auto castPacket = dynamic_cast<T *>(packet.release());

      if (castPacket == nullptr) {
        mErrorPacket = std::optional(ErrorPacket{4, "Illegal TFTP operation"});
        mState = State::ERROR;
        return nullptr;
      }

      packet.release();
      return std::unique_ptr<T>(castPacket);
    }

  public:
    Connection(std::string file_path, Options::map_t options, sockaddr_in client_address, std::string transmission_mode);

    void serveDownload();

    void serveUpload();

    void cleanup();

    ~Connection() {
      close(mSocketFd);
    }
  };
}// namespace TFTP


#endif//ISA_PROJECT_CONNECTION_H
