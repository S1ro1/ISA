//
// Created by Matej Sirovatka on 19.11.2023.
//

#ifndef ISA_PROJECT_COMMON_H
#define ISA_PROJECT_COMMON_H

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

  enum class State {
    INIT,
    SENT_RRQ,
    RECEIVED_RRQ,
    DATA_TRANSFER,
    SENT_WRQ,
    RECEIVED_WRQ,
    FINAL_ACK,
    ERROR,
    FINISHED
  };
}// namespace TFTP

#endif//ISA_PROJECT_COMMON_H
