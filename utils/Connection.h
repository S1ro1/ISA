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
#include <filesystem>

#include "ArgParser.h"
#include "TFTPPacket.h"
#include "utils.h"


class Connection {
  int mSocketFd;
  sockaddr_in mConnectionAddr;
  sockaddr_in mClientAddr;
  uint16_t mConnectionPort;

  TFTPState mState;
  Mode mMode;
  uint16_t mBlockNumber = 0;

  std::optional<ErrorPacket> mErrorPacket;
  std::unique_ptr<TFTPPacket> mLastPacket;
  bool mReachedTimeout = false;

  std::string mFilePath;

  OptionsMap mOptions;

  void sendPacket(const TFTPPacket &packet);

  std::unique_ptr<TFTPPacket> receivePacket();


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

namespace TFTPConnection {
  class TimeoutException : public std::runtime_error {
  public:
    TimeoutException() : std::runtime_error("Timeout") {}
  };

  class UndefinedException : public std::runtime_error {
  public:
    UndefinedException() : std::runtime_error("Undefined error") {}
  };
}


#endif//ISA_PROJECT_CONNECTION_H
