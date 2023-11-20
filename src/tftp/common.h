// Matej Sirovatka, xsirov00

#ifndef ISA_PROJECT_COMMON_H
#define ISA_PROJECT_COMMON_H

namespace TFTP {
  /**
   * @brief Exception after recvfrom timeout
   */
  class TimeoutException final : public std::runtime_error {
  public:
    TimeoutException() : runtime_error("Timeout") {}
  };

  /**
   * @brief Exception after recvfrom undefined error
   */
  class UndefinedException final : public std::runtime_error {
  public:
    UndefinedException() : runtime_error("Undefined error") {}
  };

  /**
   * @brief Exception after receiving packet from invalid source
   */
  class InvalidTIDException final : public std::runtime_error {
  public:
    InvalidTIDException() : runtime_error("Invalid TID") {}
  };

  /**
   * @brief Exception after receiving incorrectly formatted packet
   */
  class PacketFormatException : public std::runtime_error {
  public:
    PacketFormatException() : runtime_error("Invalid packet format") {}
  };

  /**
   * @brief State of the tftp exchange
   */
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
