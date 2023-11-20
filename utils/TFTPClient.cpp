//
// Created by Matej Sirovatka on 09.10.2023.
//

#include "TFTPClient.h"

volatile sig_atomic_t running = 1;

void siginthandler([[maybe_unused]] int signum) {
  running = 0;
}

void TFTPClient::transmit() {
  if (mMode == Mode::DOWNLOAD) {
    requestRead();
  } else {
    requestWrite();
  }
}

TFTPClient::TFTPClient(const ClientArgs &args, Options::map_t opts) : mOptions(std::move(opts)) {
  mSocketFd = socket(AF_INET, SOCK_DGRAM, 0);

  mClientAddress = {};

  mClientAddress.sin_family = AF_INET;
  mClientAddress.sin_port = htons(0);
  mClientAddress.sin_addr.s_addr = htonl(INADDR_ANY);

  socklen_t client_len = sizeof(mClientAddress);

  bind(mSocketFd, (struct sockaddr *) &mClientAddress, client_len);

  getsockname(mSocketFd, (struct sockaddr *) &mClientAddress, &client_len);

  // Set up the SIGINT handler
  struct sigaction sa {};
  memset(&sa, 0, sizeof(sa));
  sa.sa_handler = siginthandler;
  sigaction(SIGINT, &sa, nullptr);

  // remove SA_RESTART from the SIGINT handler
  sa.sa_flags &= ~SA_RESTART;
  sigaction(SIGINT, &sa, nullptr);

  mClientPort = mClientAddress.sin_port;
  mTransmissionMode = "octet";

  mBlockNumber = 1;
  mDestFilePath = args.dst_file_path;
  mState = TFTPState::INIT;
  mErrorPacket = std::nullopt;

  if (args.src_file_path.has_value()) {
    mMode = Mode::DOWNLOAD;
    mSrcFilePath = args.src_file_path.value();
  } else
    mMode = Mode::UPLOAD;

  memset(&mServerAddress, 0, sizeof(mServerAddress));

  mServerAddress.sin_family = AF_INET;
  mServerAddress.sin_port = htons(args.port);
  inet_pton(AF_INET, args.address.c_str(), &mServerAddress.sin_addr);
}

void TFTPClient::sendPacket(const TFTPPacket &packet) {
  std::vector<uint8_t> data = packet.serialize();
  sendto(mSocketFd, data.data(), data.size(), 0, (struct sockaddr *) &mServerAddress,
                        sizeof(mServerAddress));

}

std::unique_ptr<TFTPPacket> TFTPClient::receivePacket() {
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
  std::unique_ptr<TFTPPacket> packet;
  try {
    packet = TFTPPacket::deserialize(buffer);
  } catch (TFTP::PacketFormatException &e) {
    throw e;
  }

  std::cerr << packet->formatPacket(inet_ntoa(from_address.sin_addr), ntohs(from_address.sin_port),
                                    ntohs(mClientPort));

  if (mState == TFTPState::SENT_RRQ || mState == TFTPState::SENT_WRQ) {
    mServerAddress.sin_port = from_address.sin_port;
    mServerAddress.sin_addr.s_addr = from_address.sin_addr.s_addr;
    mState = TFTPState::DATA_TRANSFER;
  }

  if (mServerAddress.sin_port != from_address.sin_port || mServerAddress.sin_addr.s_addr != from_address.sin_addr.s_addr) {
    throw TFTP::InvalidTIDException();
  }

  return packet;
}

void TFTPClient::requestRead() {
  std::unique_ptr<IOutputWrapper> outputFile;
  if (mTransmissionMode == "octet") {
    outputFile = std::make_unique<Octet::OutputFile>(mDestFilePath);
  } else {
    outputFile = std::make_unique<NetAscii::OutputFile>(mDestFilePath);
  }

  mState = TFTPState::SENT_RRQ;
  Options::map_t opts = {};
  mLastPacket = std::make_unique<RRQPacket>(mSrcFilePath, mTransmissionMode, opts);

  while (mState != TFTPState::ERROR && mState != TFTPState::FINAL_ACK && running) {

    auto packet = exchangePackets(*mLastPacket, true);
    if (!packet) {
      break;
    }
    auto data_packet = dynamic_cast<DataPacket *>(packet.get());
    auto error_packet = dynamic_cast<ErrorPacket *>(packet.get());
    if (error_packet) {
      mState = TFTPState::ERROR;
      break;
    }

    if (!data_packet) {
      mState = TFTPState::ERROR;
      mErrorPacket = std::optional(ErrorPacket{4, "Illegal TFTP operation"});
      break;
    } else if (data_packet->getBlockNumber() == mBlockNumber) {
      outputFile->write(data_packet->getData());
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
      mLastPacket = std::make_unique<ACKPacket>(mBlockNumber - 1);
    }
  }

  if (mState == TFTPState::FINAL_ACK) {
    auto ack_packet = ACKPacket(mBlockNumber - 1);
    sendPacket(ack_packet);
  } else if (mErrorPacket.has_value()) {
    sendPacket(*mErrorPacket);
  }
}

void TFTPClient::requestWrite() {
  std::unique_ptr<IInputWrapper> inputFile;
  if (mTransmissionMode == "octet") {
    inputFile = std::make_unique<Octet::InputStdin>();
  } else {
    inputFile = std::make_unique<NetAscii::InputStdin>();
  }
  mState = TFTPState::SENT_WRQ;
  Options::map_t opts = {};
  mLastPacket = std::make_unique<WRQPacket>(mDestFilePath, mTransmissionMode, opts);
  mBlockNumber = 0;

  bool toSend = true;

  while (mState != TFTPState::ERROR && mState != TFTPState::FINAL_ACK && running) {
    auto packet = exchangePackets(*mLastPacket, toSend);

    if (!packet) {
      break;
    }

    auto ack_packet = dynamic_cast<ACKPacket *>(packet.get());
    auto error_packet = dynamic_cast<ErrorPacket *>(packet.get());
    if (error_packet) {
      mState = TFTPState::ERROR;
      break;
    }

    if (!ack_packet) {
      mState = TFTPState::ERROR;
      mErrorPacket = std::optional(ErrorPacket{4, "Illegal TFTP operation"});
      break;
    } else if (ack_packet->getBlockNumber() == mBlockNumber) {

      auto sent_packet = dynamic_cast<DataPacket *>(mLastPacket.get());
      if (sent_packet && sent_packet->getData().size() < Options::get("blksize", mOptions)) {
        mState = TFTPState::FINAL_ACK;
        break;
      }

      mBlockNumber++;
      toSend = true;
      std::vector<uint8_t> data(Options::get("blksize", mOptions));
      inputFile->read(reinterpret_cast<char *>(data.data()), data.size());
      data.resize(std::cin.gcount());
      mLastPacket = std::make_unique<DataPacket>(mBlockNumber, data);
    } else if (ack_packet->getBlockNumber() > mBlockNumber) {
      mState = TFTPState::ERROR;
      mErrorPacket = std::optional(ErrorPacket{4, "Illegal TFTP operation"});
      break;
    } else if (ack_packet->getBlockNumber() < mBlockNumber) {
      toSend = false;
    }
  }

  if (mErrorPacket.has_value()) {
    sendPacket(*mErrorPacket);
  }
}

std::unique_ptr<TFTPPacket> TFTPClient::exchangePackets(const TFTPPacket &packet, bool send) {
  if (send) sendPacket(packet);
  try {
    return receivePacket();
  } catch (TFTP::TimeoutException &e) {
    mErrorPacket = std::optional(ErrorPacket{0, "Timeout"});
  } catch (TFTP::InvalidTIDException &e) {
    mErrorPacket = std::optional(ErrorPacket{5, "Unknown transfer ID"});
  } catch (TFTP::UndefinedException &e) {
    mErrorPacket = std::optional(ErrorPacket{0, "Undefined error"});
  } catch (TFTP::PacketFormatException &e) {
    mErrorPacket = std::optional(ErrorPacket{4, "Illegal TFTP operation"});
  }
  mState = TFTPState::ERROR;
  return nullptr;
}
