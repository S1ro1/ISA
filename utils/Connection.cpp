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

  bind(mSocketFd, (struct sockaddr *) &mConnectionAddr, sizeof(mConnectionAddr));
  socklen_t connection_len = sizeof(mConnectionAddr);
  getsockname(mSocketFd, (struct sockaddr *) &mConnectionAddr, &connection_len);
  mConnectionPort = mConnectionAddr.sin_port;

  errorPacket = std::nullopt;
}

void Connection::serveDownload() {
  mState = TFTPState::RECEIVED_RRQ;
  std::ifstream input_file(mFilePath, std::ios::binary);

  if (!input_file.is_open()) {
    errorPacket = std::optional(ErrorPacket{1, "File not found"});
    mState = TFTPState::ERROR;
  }

  while (mState != TFTPState::FINAL_ACK and mState != TFTPState::ERROR) {
    std::vector<char> buffer(512);

    input_file.read(buffer.data(), 512);

    if (input_file.gcount() < 512) {
      buffer.resize(input_file.gcount());
    }

    DataPacket data_packet{mBlockNumber, std::vector<uint8_t>(buffer.begin(), buffer.end())};

    sendPacket(data_packet);

    std::unique_ptr<TFTPPacket> packet;
    try {
      packet = receivePacket();
    } catch (std::runtime_error &e) {
      mState = TFTPState::ERROR;
      break;
    }

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

  if (errorPacket.has_value()) {
    sendPacket(*errorPacket);
  }
}

void Connection::serveUpload() {
  mState = TFTPState::RECEIVED_WRQ;
  if (std::filesystem::exists(mFilePath)) {
    errorPacket = std::optional(ErrorPacket{6, "File already exists"});
    mState = TFTPState::ERROR;
  }

  std::ofstream output_file(mFilePath, std::ios::binary);

  while (mState != TFTPState::FINAL_ACK and mState != TFTPState::ERROR) {
    auto ack_packet = ACKPacket{mBlockNumber};
    sendPacket(ack_packet);
    mBlockNumber++;

    std::unique_ptr<TFTPPacket> packet;
    try {
      packet = receivePacket();
    } catch (std::runtime_error &e) {
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

    auto data = data_packet->getData();

    if (data_packet->getBlockNumber() == mBlockNumber) {
      output_file.write(reinterpret_cast<char *>(data.data()), static_cast<long>(data.size()));


      if (data_packet->getData().size() < 512) {
        mState = TFTPState::FINAL_ACK;
      }
    }

  }
  if (errorPacket.has_value()) {
    sendPacket(*errorPacket);
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
      // handle timeout
    } else {
      throw std::runtime_error("Error while receiving packet");
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
