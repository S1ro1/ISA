// Matej Sirovatka, xsirov00

#ifndef ISA_PROJECT_CLIENT_H
#define ISA_PROJECT_CLIENT_H

#include <arpa/inet.h>
#include <csignal>
#include <netinet/in.h>
#include <sys/socket.h>

#include "../utils/ArgParser.h"
#include "../utils/IInputWrapper.h"
#include "../utils/IOutputWrapper.h"
#include "../utils/Options.h"
#include "../utils/utils.h"
#include "Packet.h"
#include "common.h"

namespace TFTP {
  /**
   * @brief Client class
   */
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

    /**
     * @brief Sends packet to the server
     * @param packet packet to be sent
     */
    void sendPacket(const Packet &packet);

    /**
     * @brief Receives packet from the server
     * @return unique pointer to the packet received
     */
    std::unique_ptr<Packet> receivePacket();

  public:
    /**
     * @brief Client constructor
     * @param args structure holding arguments passed to the program
     * @param opts options to be used in rrq/wrq packets
     */
    explicit Client(const ClientArgs &args, Options::map_t opts);

    ~Client() {
      close(mSocketFd);
    }

    /**
     * @brief Starts the client
     */
    void transmit();

    /**
     * @brief Initializes Read request to the server
     */
    void requestRead();

    /**
     * @brief Initializes Write request to the server
     */
    void requestWrite();

    /**
     * @brief sends packet to the server if send is true, and then waits for response
     * @param packet packet to be sent
     * @param send if true, packet is sent to the server
     * @return unique pointer to the packet received, null if error occured
     */
    std::unique_ptr<Packet> exchangePackets(const Packet &packet, bool send);
  };
}// namespace TFTP


#endif//ISA_PROJECT_CLIENT_H
