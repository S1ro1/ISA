//
// Created by Matej Sirovatka on 14.10.2023.
//

#include "Connection.h"

#include <utility>

Connection::Connection(std::string file, Options::map_t options, sockaddr_in client_address) {
  mFilePath = std::move(file);
  mOptions = std::move(options);

  mBlockNumber = 0;
  mState = TFTPState::INIT;
  mSocketFd = socket(AF_INET, SOCK_DGRAM, 0);
  mClientAddr = client_address;

  mErrorPacket = std::nullopt;

  mConnectionAddr = {};
  mConnectionAddr.sin_family = AF_INET;
  mConnectionAddr.sin_port = htons(0);
  mConnectionAddr.sin_addr.s_addr = htonl(INADDR_ANY);

  struct timeval read_timeout;
  read_timeout.tv_sec = Options::get("timeout", mOptions);
  read_timeout.tv_usec = 0;

  setsockopt(mSocketFd, SOL_SOCKET, SO_RCVTIMEO, &read_timeout, sizeof read_timeout);

  bind(mSocketFd, (struct sockaddr *) &mConnectionAddr, sizeof(mConnectionAddr));
  socklen_t connection_len = sizeof(mConnectionAddr);
  getsockname(mSocketFd, (struct sockaddr *) &mConnectionAddr, &connection_len);
  mConnectionPort = mConnectionAddr.sin_port;

  mLastPacket = nullptr;
}

void Connection::serveDownload() {
  mState = TFTPState::RECEIVED_RRQ;
  std::ifstream input_file(mFilePath, std::ios::binary);

  if (!input_file.is_open()) {
    sendPacket(ErrorPacket{1, "File not found"});
    return;
  }

  mBlockNumber = 0;
  bool send = true;
  while (mState != TFTPState::FINAL_ACK and mState != TFTPState::ERROR) {
    std::vector<char> buffer(65536);


    if (mBlockNumber == 0) {
      if (Options::isSet(mOptions)) {
        mBlockNumber = 0;
        mLastPacket = std::make_unique<OACKPacket>(mOptions);
      } else {
        mBlockNumber = 1;
        input_file.read(buffer.data(), Options::get("blksize", mOptions));
        buffer.resize(input_file.gcount());
        mLastPacket = std::make_unique<DataPacket>(mBlockNumber, std::vector<uint8_t>(buffer.begin(), buffer.end()));
      }
    } else {
      input_file.read(buffer.data(), Options::get("blksize", mOptions));
      buffer.resize(input_file.gcount());
      mLastPacket = std::make_unique<DataPacket>(mBlockNumber, std::vector<uint8_t>(buffer.begin(), buffer.end()));
    }

    auto packet = sendAndReceive(*mLastPacket, send);

    auto error_packet = dynamic_cast<ErrorPacket *>(packet.get());
    if (error_packet) {
      break;
    }

    auto ack_packet = expectPacketType<ACKPacket>(std::move(packet));

    if (!ack_packet) break;

    auto blockNum = ack_packet->getBlockNumber();
    if (blockNum == mBlockNumber) {
      mBlockNumber++;
      send = true;
      if (input_file.eof()) break;

    } else if (blockNum > mBlockNumber) {
      mState = TFTPState::ERROR;
      mErrorPacket = std::optional(ErrorPacket{4, "Illegal TFTP operation"});
      break;
    } else if (blockNum < mBlockNumber) {
      send = false;
    }
  }
  input_file.close();

  if (mErrorPacket.has_value()) {
    sendPacket(*mErrorPacket);
  }

  mState = TFTPState::FINISHED;
}

void Connection::serveUpload() {
  mState = TFTPState::RECEIVED_WRQ;
  if (std::filesystem::exists(mFilePath)) {
    sendPacket(ErrorPacket{6, "File already exists"});
    return;
  }

  mBlockNumber = 1;
  mLastPacket = std::make_unique<ACKPacket>(mBlockNumber - 1);

  Options::map_t oack_options;
  for (auto &[order, item] : mOptions) {
    auto &[key, value, set] = item;
    if (set) oack_options[order] = item;
  }
  if (!oack_options.empty()) mLastPacket = std::make_unique<OACKPacket>(oack_options);

  std::ofstream output_file(mFilePath, std::ios::binary);
  if (!output_file.is_open() or !output_file.good()) {
    sendPacket(ErrorPacket{2, "Access violation"});
    return;
  }

  while (mState != TFTPState::FINAL_ACK && mState != TFTPState::ERROR) {
    auto packet = sendAndReceive(*mLastPacket, true);

    if (!packet) break;

    auto error_packet = dynamic_cast<ErrorPacket *>(packet.get());
    if (error_packet) {
      break;
    }

    auto data_packet = expectPacketType<DataPacket>(std::move(packet));

    if (!data_packet) break;

    auto data = data_packet->getData();
    if (data_packet->getBlockNumber() == mBlockNumber) {
      output_file.write(reinterpret_cast<char *>(data.data()), static_cast<long>(data.size()));
      if (data_packet->getData().size() < Options::get("blksize", mOptions)) {
        mState = TFTPState::FINAL_ACK;
      }
      // Increment block number only after it is valid packet
      mBlockNumber++;
      mLastPacket = std::make_unique<ACKPacket>(mBlockNumber - 1);
    } else if (data_packet->getBlockNumber() > mBlockNumber) {
      mState = TFTPState::ERROR;
      mErrorPacket = std::optional(ErrorPacket{4, "Illegal TFTP operation"});
      break;
    } else if (data_packet->getBlockNumber() < mBlockNumber) {
      mLastPacket = std::make_unique<ACKPacket>(mBlockNumber);
    }
  }

  // Success
  if (mState == TFTPState::FINAL_ACK) {
    auto ack_packet = ACKPacket(mBlockNumber - 1);
    sendPacket(ack_packet);
    output_file.close();
    return;
  }

  // mState should only be ERROR here
  if (mErrorPacket.has_value()) {
    sendPacket(*mErrorPacket);
  }
  std::filesystem::remove(mFilePath);
}

void Connection::sendPacket(const TFTPPacket &packet) {

  std::cout << "Sending packet: " << packet.formatPacket(inet_ntoa(mClientAddr.sin_addr), ntohs(mClientAddr.sin_port),
                                    ntohs(mConnectionPort)) << "with size: " << packet.serialize().size() << std::endl;


  std::vector<uint8_t> data = packet.serialize();
  // TODO: Error handling
  ssize_t sent = sendto(mSocketFd, data.data(), data.size(), 0, (struct sockaddr *) &mClientAddr,
                        sizeof(mClientAddr));
}

std::unique_ptr<TFTPPacket> Connection::receivePacket() const {
  std::vector<uint8_t> buffer(65535);

  sockaddr_in from_address = {};

  socklen_t from_length = sizeof(from_address);

  ssize_t received = recvfrom(mSocketFd, buffer.data(), buffer.size(), 0, (struct sockaddr *) &from_address,
                              &from_length);

  if (received <= 0) {
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
      throw TFTPConnection::TimeoutException();
    } else {
      throw TFTPConnection::UndefinedException();
    }
  } else {
    if (from_address.sin_port != mClientAddr.sin_port || from_address.sin_addr.s_addr != mClientAddr.sin_addr.s_addr) {
      throw TFTPConnection::InvalidTIDException();
    }
  }

  buffer.resize(received);

  std::unique_ptr<TFTPPacket> packet;

  try {
    packet = TFTPPacket::deserialize(buffer);
  } catch (TFTPFormatError &e) {
    throw e;
  }

  std::cerr << packet->formatPacket(inet_ntoa(mClientAddr.sin_addr), ntohs(mClientAddr.sin_port),
                                    ntohs(mConnectionPort));
  return packet;
}

void Connection::cleanup() {
  if (mState != TFTPState::FINISHED) {
    sendPacket(ErrorPacket{0, "Server shutting down"});
  }
}

/**
 * Sends packet depending on send param, waits for 3 timeouts and returns it, returns nullptr and sets error packet accordingly in case of failure
 * @param packetToSend packet to be sent
 * @param send whether to send packet, or just wait
 * @return received, packet, nullptr in case of failure
 */
std::unique_ptr<TFTPPacket> Connection::sendAndReceive(const TFTPPacket& packetToSend, bool send) {
  int retries = 0;
  while (retries < 3) {
    if (send) {
      sendPacket(packetToSend);
    }
    try {
      return receivePacket();
    } catch (TFTPConnection::TimeoutException &e) {
      retries++;
      if (retries == 3) {
        mErrorPacket = ErrorPacket(0, "Timeout");
        break;
      }
      continue;
    } catch (TFTPConnection::UndefinedException &e) {
      mErrorPacket = ErrorPacket(0, "Undefined error");
      break;
    } catch (TFTPConnection::InvalidTIDException &e) {
      mErrorPacket = ErrorPacket(5, "Unknown transfer ID");
      break;
    } catch (TFTPFormatError &e) {
      mErrorPacket = ErrorPacket(4, "Illegal TFTP operation");
      break;
    }
  }
  mState = TFTPState::ERROR;
  return nullptr;
}