// Matej Sirovatka, xsirov00

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
  /**
   * @brief Connection class, handles the while tftp exchange with a single client on server side
   */
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

    /**
     * @brief Sends packet to the client
     * @param packet packet to be sent
     */
    void sendPacket(const Packet &packet);

    /**
     * @brief Receives packet from the client
     * @return unique pointer to the received packet
     */
    [[nodiscard]] std::unique_ptr<Packet> receivePacket() const;

    /**
     * @brief sends packet to the client if send is true, and then waits for response
     * @param packetToSend packet to be sent
     * @param send whether to send the packet, or just wait for response
     * @return unique pointer to the received packet, null if error occured
     */
    std::unique_ptr<Packet> sendAndReceive(const Packet &packetToSend, bool send);

    /**
     * expects packet of type T, if the packet is not of type T, sends error packet and sets state to ERROR,
     * if it is of type T, releases it and returns new unique pointer downcast to T
     * @tparam T expected type
     * @param packet packet to be checked
     * @return unique pointer of type T, nullptr if error occured
     */
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
    /**
     * @brief Connection constructor
     * @param file_path file path from where path should be read/written to
     * @param options options to be used in exchange
     * @param client_address client address
     * @param transmission_mode mode of transmission, either netascii or octet
     */
    Connection(std::string file_path, Options::map_t options, sockaddr_in client_address, std::string transmission_mode);

    /**
     * @brief Handles downloading from server
     */
    void serveDownload();

    /**
     * @brief Handles uploading to server
     */
    void serveUpload();

    /**
     * @brief Cleans up the connection incase of it not being successful
     */
    void cleanup();

    ~Connection() {
      close(mSocketFd);
    }
  };
}// namespace TFTP


#endif//ISA_PROJECT_CONNECTION_H
