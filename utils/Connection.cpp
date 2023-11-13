//
// Created by Matej Sirovatka on 14.10.2023.
//

#include "Connection.h"

Connection::Connection(std::string file, OptionsMap options, sockaddr_in client_address) : mOptions(std::move(options)) {
  mFilePath = std::move(file);

  mState = TFTPState::INIT;
  mSocketFd = socket(AF_INET, SOCK_DGRAM, 0);
  mClientAddr = client_address;

  mConnectionAddr = {};
  mConnectionAddr.sin_family = AF_INET;
  mConnectionAddr.sin_port = htons(0);
  mConnectionAddr.sin_addr.s_addr = htonl(INADDR_ANY);

  struct timeval read_timeout;
  read_timeout.tv_sec = mOptions.mTimeout.first;
  read_timeout.tv_usec = 0;

  int ret = setsockopt(mSocketFd, SOL_SOCKET, SO_RCVTIMEO, &read_timeout, sizeof read_timeout);

  bind(mSocketFd, (struct sockaddr *) &mConnectionAddr, sizeof(mConnectionAddr));
  socklen_t connection_len = sizeof(mConnectionAddr);
  getsockname(mSocketFd, (struct sockaddr *) &mConnectionAddr, &connection_len);
  mConnectionPort = mConnectionAddr.sin_port;

  mErrorPacket = std::nullopt;
  mLastPacket = nullptr;
}

void Connection::serveDownload() {
  mState = TFTPState::RECEIVED_RRQ;
  std::ifstream input_file(mFilePath, std::ios::binary);

  if (!input_file.is_open()) {
    mErrorPacket = std::optional(ErrorPacket{1, "File not found"});
    mState = TFTPState::ERROR;
  }

  while (mState != TFTPState::FINAL_ACK and mState != TFTPState::ERROR) {

    if (!mReachedTimeout) {
      std::vector<char> buffer(65500);

      input_file.read(buffer.data(), mOptions.mBlksize.first);

      if (input_file.gcount() < mOptions.mBlksize.first) {
        buffer.resize(input_file.gcount());
      }

      mLastPacket = std::make_unique<DataPacket>(mBlockNumber, std::vector<uint8_t>(buffer.begin(), buffer.end()));
    }

    sendPacket(*mLastPacket);

    std::unique_ptr<TFTPPacket> packet;
    try {
      packet = receivePacket();
    } catch (TFTPConnection::TimeoutException &e) {
      mReachedTimeout = true;
      continue;
    }
    mReachedTimeout = false;

    auto ack_packet = dynamic_cast<ACKPacket *>(packet.get());
    auto error_packet = dynamic_cast<ErrorPacket *>(packet.get());

    if (ack_packet) {
      if (ack_packet->getBlockNumber() == mBlockNumber) {
        mBlockNumber++;
      }

      if (input_file.eof()) {
        mState = TFTPState::FINAL_ACK;
      }
    } else if (error_packet) {
      mState = TFTPState::ERROR;
    }
  }

  input_file.close();

  if (mErrorPacket.has_value()) {
    sendPacket(*mErrorPacket);
  }
}

void Connection::serveUpload() {
  mState = TFTPState::RECEIVED_WRQ;
  if (std::filesystem::exists(mFilePath)) {
    mErrorPacket = std::optional(ErrorPacket{6, "File already exists"});
    mState = TFTPState::ERROR;
  }

  std::ofstream output_file(mFilePath, std::ios::binary);

  while (mState != TFTPState::FINAL_ACK and mState != TFTPState::ERROR) {

    if (!mReachedTimeout) {
      mLastPacket = std::make_unique<ACKPacket>(mBlockNumber);
    }

    sendPacket(*mLastPacket);

    std::unique_ptr<TFTPPacket> packet;
    try {
      packet = receivePacket();
    } catch (TFTPConnection::TimeoutException &e) {
      mReachedTimeout = true;
      continue;
    } catch (TFTPConnection::UndefinedException &e) {
      mState = TFTPState::ERROR;
      break;
    }

    auto data_packet = dynamic_cast<DataPacket *>(packet.get());
    auto error_packet = dynamic_cast<ErrorPacket *>(packet.get());

    if (error_packet) {
      mState = TFTPState::ERROR;
    } else if (!data_packet) {
      mState = TFTPState::ERROR;
    }

    mReachedTimeout = false;
    // TODO: Check if block number is correct
    mBlockNumber++;

    auto data = data_packet->getData();

    if (data_packet->getBlockNumber() == mBlockNumber) {
      output_file.write(reinterpret_cast<char *>(data.data()), static_cast<long>(data.size()));


      if (data_packet->getData().size() < mOptions.mBlksize.first) {
        mState = TFTPState::FINAL_ACK;
      }
    }

  }
  if (mErrorPacket.has_value()) {
    sendPacket(*mErrorPacket);
  } else {
    auto ack_packet = ACKPacket{mBlockNumber};
    sendPacket(ack_packet);
  }

  output_file.close();
}

void Connection::sendPacket(const TFTPPacket &packet) {
  std::vector<uint8_t> data = packet.serialize();


  // TODO: Error handling
  ssize_t sent = sendto(mSocketFd, data.data(), data.size(), 0, (struct sockaddr *) &mClientAddr,
                        sizeof(mClientAddr));

}

std::unique_ptr<TFTPPacket> Connection::receivePacket() {
  std::vector<uint8_t> buffer(65535);

  socklen_t from_length = sizeof(mClientAddr);

  ssize_t received = recvfrom(mSocketFd, buffer.data(), buffer.size(), 0, (struct sockaddr *) &mClientAddr,
                              &from_length);

  // TODO: Check this outside of the scope
  if (received <= 0) {
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
      throw TFTPConnection::TimeoutException();
    } else {
      throw TFTPConnection::UndefinedException();
    }
  }

  buffer.resize(received);

  auto packet = TFTPPacket::deserialize(buffer);

  std::cerr << packet->formatPacket(inet_ntoa(mClientAddr.sin_addr), ntohs(mClientAddr.sin_port),
                                    ntohs(mConnectionPort));
  return packet;
}
void Connection::cleanup() {
  if (mState != TFTPState::FINISHED) {
    std::filesystem::remove(mFilePath);
    sendPacket(ErrorPacket{0, "Server shutting down"});
  }
}
