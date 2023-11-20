//
// Created by Matej Sirovatka on 14.10.2023.
//

#include "Connection.h"

void ConnectionSigUsrHandler(int signum) {
  return;
}

TFTP::Connection::Connection(std::string file, Options::map_t options, sockaddr_in client_address, std::string transmission_mode) {
  mFilePath = std::move(file);
  mOptions = std::move(options);
  mTransmissionMode = std::move(transmission_mode);

  struct sigaction sa;
  sa.sa_handler = ConnectionSigUsrHandler;

  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;
  sigaction(SIGUSR1, &sa, NULL);

  mBlockNumber = 0;
  mState = State::INIT;
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

void TFTP::Connection::serveDownload() {
  mState = State::RECEIVED_RRQ;

  std::unique_ptr<IInputWrapper> input_file;
  if (mTransmissionMode == "octet") {
    input_file = std::make_unique<Octet::InputFile>(mFilePath);
  } else if (mTransmissionMode == "netascii") {
    input_file = std::make_unique<NetAscii::InputFile>(mFilePath);
  } else {
    sendPacket(ErrorPacket{4, "Illegal TFTP operation"});
    mState = State::FINISHED;
    return;
  }

  if (!input_file->is_open()) {
    sendPacket(ErrorPacket{1, "File not found"});
    mState = State::FINISHED;
    return;
  }

  mBlockNumber = 0;
  bool send = true;
  while (mState != State::FINAL_ACK and mState != State::ERROR) {
    std::vector<char> buffer(65536);

    if (mBlockNumber == 0) {
      if (Options::isAny(mOptions)) {
        mBlockNumber = 0;
        Options::map_t oack_options;
        for (auto &[order, item]: mOptions) {
          auto &[key, value, set] = item;
          if (set) oack_options[order] = item;
        }
        long fs = std::filesystem::file_size(mFilePath);
        if (Options::isSet("tsize", mOptions)) oack_options[2] = std::tuple("tsize", fs, true);

        mLastPacket = std::make_unique<OACKPacket>(oack_options);
      } else {
        mBlockNumber = 1;
        input_file->read(buffer.data(), Options::get("blksize", mOptions));
        buffer.resize(input_file->gcount());
        mLastPacket = std::make_unique<DataPacket>(mBlockNumber, std::vector<uint8_t>(buffer.begin(), buffer.end()));
      }
    } else {
      input_file->read(buffer.data(), Options::get("blksize", mOptions));
      buffer.resize(input_file->gcount());
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
      if (input_file->eof()) break;

    } else if (blockNum > mBlockNumber) {
      mState = State::ERROR;
      mErrorPacket = std::optional(ErrorPacket{4, "Illegal TFTP operation"});
      break;
    } else if (blockNum < mBlockNumber) {
      send = false;
    }
  }

  if (mErrorPacket.has_value()) {
    sendPacket(*mErrorPacket);
  }

  mState = State::FINISHED;
}

void TFTP::Connection::serveUpload() {
  mState = State::RECEIVED_WRQ;
  if (std::filesystem::exists(mFilePath)) {
    sendPacket(ErrorPacket{6, "File already exists"});
    mState = State::FINISHED;
    return;
  }

  mBlockNumber = 1;
  mLastPacket = std::make_unique<ACKPacket>(mBlockNumber - 1);

  Options::map_t oack_options;
  for (auto &[order, item]: mOptions) {
    auto &[key, value, set] = item;
    if (set) oack_options[order] = item;
  }
  if (!oack_options.empty()) mLastPacket = std::make_unique<OACKPacket>(oack_options);

  std::unique_ptr<IOutputWrapper> output_file;

  if (mTransmissionMode == "octet") {
    output_file = std::make_unique<Octet::OutputFile>(mFilePath);
  } else if (mTransmissionMode == "netascii") {
    output_file = std::make_unique<NetAscii::OutputFile>(mFilePath);
  } else {
    sendPacket(ErrorPacket{4, "Illegal TFTP operation"});
    mState = State::FINISHED;
    return;
  }

  if (!output_file->is_open() or !output_file->good()) {
    sendPacket(ErrorPacket{2, "Access violation"});
    mState = State::FINISHED;
    return;
  }

  while (mState != State::FINAL_ACK && mState != State::ERROR) {
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
      output_file->write(data_packet->getData());
      if (data_packet->getData().size() < Options::get("blksize", mOptions)) {
        mState = State::FINAL_ACK;
      }
      // Increment block number only after it is valid packet
      mBlockNumber++;
      mLastPacket = std::make_unique<ACKPacket>(mBlockNumber - 1);
    } else if (data_packet->getBlockNumber() > mBlockNumber) {
      mState = State::ERROR;
      mErrorPacket = std::optional(ErrorPacket{4, "Illegal TFTP operation"});
      break;
    } else if (data_packet->getBlockNumber() < mBlockNumber) {
      mLastPacket = std::make_unique<ACKPacket>(mBlockNumber - 1);
    }
  }

  // Success
  if (mState == State::FINAL_ACK) {
    auto ack_packet = ACKPacket(mBlockNumber - 1);
    sendPacket(ack_packet);
    mState = State::FINISHED;
    return;
  }

  // mState should only be ERROR here
  if (mErrorPacket.has_value()) {
    sendPacket(*mErrorPacket);
    mState = State::FINISHED;
  }
  std::filesystem::remove(mFilePath);
}

void TFTP::Connection::sendPacket(const Packet &packet) {
  std::vector<uint8_t> data = packet.serialize();
  sendto(mSocketFd, data.data(), data.size(), 0, (struct sockaddr *) &mClientAddr,
         sizeof(mClientAddr));
}

std::unique_ptr<TFTP::Packet> TFTP::Connection::receivePacket() const {
  std::vector<uint8_t> buffer(65535);
  sockaddr_in from_address = {};
  socklen_t from_length = sizeof(from_address);
  ssize_t received = recvfrom(mSocketFd, buffer.data(), buffer.size(), 0, (struct sockaddr *) &from_address,
                              &from_length);


  if (received <= 0) {
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
      throw TFTP::TimeoutException();
    } else {
      throw TFTP::UndefinedException();
    }
  }

  buffer.resize(received);
  std::unique_ptr<Packet> packet;
  try {
    packet = Packet::deserialize(buffer);
  } catch (TFTP::PacketFormatException &e) {
    throw e;
  }

  std::cerr << packet->formatPacket(inet_ntoa(mClientAddr.sin_addr), ntohs(mClientAddr.sin_port),
                                    ntohs(mConnectionPort));

  if (from_address.sin_port != mClientAddr.sin_port || from_address.sin_addr.s_addr != mClientAddr.sin_addr.s_addr) {
    throw TFTP::InvalidTIDException();
  }

  return packet;
}

void TFTP::Connection::cleanup() {
  if (mState != State::FINISHED) {
    sendPacket(ErrorPacket{0, "Server shutting down"});
  }
}

/**
 * Sends packet depending on send param, waits for 3 timeouts and returns it, returns nullptr and sets error packet accordingly in case of failure
 * @param packetToSend packet to be sent
 * @param send whether to send packet, or just wait
 * @return received, packet, nullptr in case of failure
 */
std::unique_ptr<TFTP::Packet> TFTP::Connection::sendAndReceive(const Packet &packetToSend, bool send) {
  int retries = 0;
  while (retries < 3) {
    if (send) {
      sendPacket(packetToSend);
    }
    try {
      return receivePacket();
    } catch (TFTP::TimeoutException &e) {
      retries++;
      if (retries == 3) {
        mErrorPacket = ErrorPacket(0, "Timeout");
        break;
      }
      continue;
    } catch (TFTP::UndefinedException &e) {
      mErrorPacket = ErrorPacket(0, "Undefined error");
      break;
    } catch (TFTP::InvalidTIDException &e) {
      mErrorPacket = ErrorPacket(5, "Unknown transfer ID");
      break;
    } catch (TFTP::PacketFormatException &e) {
      mErrorPacket = ErrorPacket(4, "Illegal TFTP operation");
      break;
    }
  }
  mState = State::ERROR;
  return nullptr;
}