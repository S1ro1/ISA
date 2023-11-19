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
#include "Options.h"
#include "TFTP.h"


class TFTPClient {
  int mSocketFd;
  sockaddr_in mServerAddress;
  sockaddr_in mClientAddress;

  uint16_t mClientPort;

  TFTPState mState;
  std::string mTransmissionMode = "octet";

  int mBlockNumber;
  Mode mMode;
  std::string mSrcFilePath;
  std::string mDestFilePath;
  std::optional<ErrorPacket> mErrorPacket;

  std::unique_ptr<TFTPPacket> mLastPacket;

  Options::map_t mOptions;

  void sendPacket(const TFTPPacket &packet);

  std::unique_ptr<TFTPPacket> receivePacket();

public:
  explicit TFTPClient(const ClientArgs &args, Options::map_t opts);

  ~TFTPClient() {
    close(mSocketFd);
  }

  void transmit();

  void requestRead();

  void requestWrite();

  void handleDataPacket(std::ofstream &outputFile, DataPacket *data_packet);

  std::unique_ptr<TFTPPacket> exchangePackets(const TFTPPacket &packet, bool send);
};


#endif//ISA_PROJECT_TFTPCLIENT_H
