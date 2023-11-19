//
// Created by Matej Sirovatka on 19.11.2023.
//

#ifndef ISA_PROJECT_TFTP_H
#define ISA_PROJECT_TFTP_H

#include "ArgParser.h"
#include "Options.h"
#include "TFTPPacket.h"
#include "utils.h"
#include <any>
#include <arpa/inet.h>
#include <array>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <netinet/in.h>
#include <set>
#include <string>
#include <sys/socket.h>
#include <unistd.h>
#include <utility>
#include <vector>

namespace TFTP {
  class TimeoutException final : public std::runtime_error {
  public:
    TimeoutException() : runtime_error("Timeout") {}
  };

  class UndefinedException final : public std::runtime_error {
  public:
    UndefinedException() : runtime_error("Undefined error") {}
  };

  class InvalidTIDException final : public std::runtime_error {
  public:
    InvalidTIDException() : runtime_error("Invalid TID") {}
  };

  class PacketFormatException : public std::runtime_error {
  public:
    PacketFormatException() : runtime_error("Invalid packet format") {}
  };
}// namespace TFTP

#endif//ISA_PROJECT_TFTP_H
