//
// Created by Matej Sirovatka on 09.10.2023.
//

#include "TFTPClient.h"
#include "TFTP.h"
#include <iomanip>

#include <chrono>
#include <thread>

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

  mClientPort = mClientAddress.sin_port;

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
  std::ofstream outputFile(mDestFilePath, std::ios::binary);
  mState = TFTPState::SENT_RRQ;
  Options::map_t opts = {};
  mLastPacket = std::make_unique<RRQPacket>(mSrcFilePath, mTransmissionMode, opts);

  while (mState != TFTPState::ERROR && mState != TFTPState::FINAL_ACK) {

    auto packet = exchangePackets(*mLastPacket, true);
    if (!packet) {
      break;
    }
    auto data_packet = dynamic_cast<DataPacket *>(packet.get());
    auto error_packet = dynamic_cast<ErrorPacket *>(packet.get());
    if (error_packet) {
      mState = TFTPState::ERROR;
      outputFile.close();
      return;
    }

    if (!data_packet) {
      mState = TFTPState::ERROR;
      mErrorPacket = std::optional(ErrorPacket{4, "Illegal TFTP operation"});
      break;
    } else if (data_packet->getBlockNumber() == mBlockNumber) {
      outputFile.write(reinterpret_cast<const char *>(data_packet->getData().data()), data_packet->getData().size());
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

  outputFile.close();
  return;
}

void TFTPClient::requestWrite() {
  WRQPacket wrq(mDestFilePath, mTransmissionMode, mOptions);

  sendPacket(wrq);
  mState = TFTPState::SENT_WRQ;

  auto packet = receivePacket();

  auto oack_packet = dynamic_cast<OACKPacket *>(packet.get());
  auto ack_packet = dynamic_cast<ACKPacket *>(packet.get());
  auto err_packet = dynamic_cast<ErrorPacket *>(packet.get());

  if (oack_packet) {
    mOptions = oack_packet->getOptions();
  } else if (ack_packet) {

  } else if (err_packet) {
    mState = TFTPState::ERROR;
    return;
  } else {
    mState = TFTPState::ERROR;
    return;
  }

  uint16_t blockNumber = 1;
  long blksize = Options::get("blksize", mOptions);

  while (mState != TFTPState::FINAL_ACK && mState != TFTPState::ERROR) {
    std::vector<uint8_t> data(Options::get("blksize", mOptions));

    std::cin.read(reinterpret_cast<char *>(data.data()), blksize);
    size_t bytesRead = std::cin.gcount();

    data.resize(bytesRead);
    DataPacket dataPacket(blockNumber, data);
    sendPacket(dataPacket);

    packet = receivePacket();
    auto ack_packet = dynamic_cast<ACKPacket *>(packet.get());
    if (!ack_packet) {
      mState = TFTPState::ERROR;
      return;
    }
    blockNumber++;
    if (bytesRead < blksize) mState = TFTPState::FINAL_ACK;
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
