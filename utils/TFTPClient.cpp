//
// Created by Matej Sirovatka on 09.10.2023.
//

#include "TFTPClient.h"
#include <iomanip>

#include <chrono>
#include <thread>

void TFTPClient::transmit() {
  if (mode == Mode::DOWNLOAD) {
    requestRead();
  } else {
    requestWrite();
  }
}

TFTPClient::TFTPClient(const ClientArgs &args, Options::map_t opts) : opts(std::move(opts)) {
  socket_fd = socket(AF_INET, SOCK_DGRAM, 0);

  client_address = {};

  client_address.sin_family = AF_INET;
  client_address.sin_port = htons(0);
  client_address.sin_addr.s_addr = htonl(INADDR_ANY);

  socklen_t client_len = sizeof(client_address);

  bind(socket_fd, (struct sockaddr *) &client_address, client_len);

  getsockname(socket_fd, (struct sockaddr *) &client_address, &client_len);

  client_port = client_address.sin_port;

  dst_file_path = args.dst_file_path;
  state = TFTPState::INIT;

  if (args.src_file_path.has_value()) {
    mode = Mode::DOWNLOAD;
    src_file_path = args.src_file_path.value();
  } else
    mode = Mode::UPLOAD;

  memset(&server_address, 0, sizeof(server_address));

  server_address.sin_family = AF_INET;
  server_address.sin_port = htons(args.port);

  if (inet_pton(AF_INET, args.address.c_str(), &server_address.sin_addr) <= 0) {
    exit(1);
  }
}

void TFTPClient::sendPacket(const TFTPPacket &packet) {
  std::vector<uint8_t> data = packet.serialize();

  sendto(socket_fd, data.data(), data.size(), 0, (struct sockaddr *) &server_address,
                        sizeof(server_address));

}

std::unique_ptr<TFTPPacket> TFTPClient::receivePacket() {
  std::vector<uint8_t> buffer(65535);

  sockaddr_in from_address = {};

  socklen_t from_length = sizeof(from_address);

  ssize_t received = recvfrom(socket_fd, buffer.data(), buffer.size(), 0, (struct sockaddr *) &from_address,
                              &from_length);

  if (state == TFTPState::SENT_RRQ || state == TFTPState::SENT_WRQ) server_address.sin_port = from_address.sin_port;
  buffer.resize(received);

  auto packet = TFTPPacket::deserialize(buffer);

  std::cerr << packet->formatPacket(inet_ntoa(from_address.sin_addr), ntohs(from_address.sin_port),
                                    ntohs(client_port));

  return packet;
}

void TFTPClient::handleDataPacket(std::ofstream &outputFile, DataPacket *data_packet) {
  uint16_t blockNumber = data_packet->getBlockNumber();
  ACKPacket ack(blockNumber);
  sendPacket(ack);

  std::vector<uint8_t> data = data_packet->getData();

  if (data.size() < Options::get("blksize", opts)) {
    state = TFTPState::FINAL_ACK;
  }
  outputFile.write(reinterpret_cast<char *>(data.data()), static_cast<long>(data.size()));
}

void TFTPClient::requestRead() {
  std::ofstream outputFile(dst_file_path, std::ios::binary);
  RRQPacket rrq(src_file_path, transmissionMode, opts);
  sendPacket(rrq);

  state = TFTPState::SENT_RRQ;

  auto packet = receivePacket();

  auto oack_packet = dynamic_cast<OACKPacket *>(packet.get());
  auto data_packet = dynamic_cast<DataPacket *>(packet.get());

  if (oack_packet) {
    opts = oack_packet->getOptions();

    ACKPacket ack{0};
    sendPacket(ack);

    packet = receivePacket();

    data_packet = dynamic_cast<DataPacket *>(packet.get());

    handleDataPacket(outputFile, data_packet);

  } else if (data_packet) {
    handleDataPacket(outputFile, data_packet);
  } else {
    state = TFTPState::ERROR;
    outputFile.close();
    return;
  }

  while (state != TFTPState::FINAL_ACK) {
//    std::this_thread::sleep_for(std::chrono::milliseconds(3000));
    auto packet = receivePacket();

    auto data_packet = dynamic_cast<DataPacket *>(packet.get());
    if (data_packet) {
      handleDataPacket(outputFile, data_packet);

      continue;
    }

    auto err_packet = dynamic_cast<ErrorPacket *>(packet.get());

    if (err_packet) {
      //TODO: error handling
      state = TFTPState::ERROR;
      outputFile.close();
      return;
    }
  }

  outputFile.close();
}

void TFTPClient::requestWrite() {
  WRQPacket wrq(dst_file_path, transmissionMode, opts);

  sendPacket(wrq);
  state = TFTPState::SENT_WRQ;

  auto packet = receivePacket();

  auto oack_packet = dynamic_cast<OACKPacket *>(packet.get());
  auto ack_packet = dynamic_cast<ACKPacket *>(packet.get());
  auto err_packet = dynamic_cast<ErrorPacket *>(packet.get());

  if (oack_packet) {
    opts = oack_packet->getOptions();
  } else if (ack_packet) {

  } else if (err_packet) {
    state = TFTPState::ERROR;
    return;
  } else {
    state = TFTPState::ERROR;
    return;
  }

  uint16_t blockNumber = 1;
  long blksize = Options::get("blksize", opts);

  while (state != TFTPState::FINAL_ACK && state != TFTPState::ERROR) {
    std::vector<uint8_t> data(Options::get("blksize", opts));

    std::cin.read(reinterpret_cast<char *>(data.data()), blksize);
    size_t bytesRead = std::cin.gcount();

    data.resize(bytesRead);
    DataPacket dataPacket(blockNumber, data);
    sendPacket(dataPacket);

    packet = receivePacket();
    auto ack_packet = dynamic_cast<ACKPacket *>(packet.get());
    if (!ack_packet) {
      state = TFTPState::ERROR;
      return;
    }
    blockNumber++;
    if (bytesRead < blksize) state = TFTPState::FINAL_ACK;
  }
}
