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
#include "Options.h"
#include "IInputWrapper.h"
#include "IOutputWrapper.h"


class Connection {
  int mSocketFd;
  uint16_t mConnectionPort;
  sockaddr_in mConnectionAddr;

  sockaddr_in mClientAddr;

  TFTPState mState;
  std::string mTransmissionMode;
  uint16_t mBlockNumber;

  std::optional<ErrorPacket> mErrorPacket;
  std::unique_ptr<TFTPPacket> mLastPacket;

  std::string mFilePath;
  Options::map_t mOptions;

  void sendPacket(const TFTPPacket &packet);

  [[nodiscard]] std::unique_ptr<TFTPPacket> receivePacket() const;

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
  Connection(std::string file_path, Options::map_t options, sockaddr_in client_address, std::string transmission_mode);

  void serveDownload();

  void serveUpload();

  void cleanup();

  ~Connection() {
    close(mSocketFd);
  }
};



#endif//ISA_PROJECT_CONNECTION_H
