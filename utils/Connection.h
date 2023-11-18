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
#include <unistd.h>
#include <utility>
#include <filesystem>

#include "ArgParser.h"
#include "TFTPPacket.h"
#include "utils.h"


namespace TFTPConnection {
  class TimeoutException final : public std::runtime_error {
  public:
    TimeoutException() : std::runtime_error("Timeout") {}
  };

  class UndefinedException final : public std::runtime_error {
  public:
    UndefinedException() : std::runtime_error("Undefined error") {}
  };
}


class Connection {
  int mSocketFd;
  sockaddr_in mConnectionAddr;
  sockaddr_in mClientAddr;
  uint16_t mConnectionPort;

  TFTPState mState;
  Mode mMode;
  uint16_t mBlockNumber;

  std::optional<ErrorPacket> mErrorPacket;
  std::unique_ptr<TFTPPacket> mLastPacket;
  bool mReachedTimeout = false;

  std::string mFilePath;

  OptionsMap mOptions;

  void sendPacket(const TFTPPacket &packet);

  std::unique_ptr<TFTPPacket> receivePacket();

  std::unique_ptr<TFTPPacket> sendAndReceive(const TFTPPacket& packetToSend, bool send);

  template <typename T>
  [[nodiscard]] std::unique_ptr<T> expectPacketType(std::unique_ptr<TFTPPacket> packet) {
    if (packet == nullptr) {
      return nullptr;
    }

    auto castPacket = dynamic_cast<T*>(packet.release());

    if (castPacket == nullptr) {
      mErrorPacket = std::optional(ErrorPacket{4, "Illegal TFTP operation"});
      mState = TFTPState::ERROR;
      return nullptr;
    }

    packet.release();
    return std::unique_ptr<T>(castPacket);
  }

public:
  Connection(std::string file_path, OptionsMap options, sockaddr_in client_address);

  void serveDownload();

  void serveUpload();

  void cleanup();

  ~Connection() {
    LOG("Closing connection...");
    close(mSocketFd);
  }
};



#endif//ISA_PROJECT_CONNECTION_H
