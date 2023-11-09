//
// Created by Matej Sirovatka on 14.10.2023.
//

#include "Connection.h"

Connection::Connection(std::string file, OptionsMap options, sockaddr_in client_address) {
  this->file_path = std::move(file);
  this->opts = std::move(options);

  state = TFTPState::INIT;
  socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
  this->client_address = client_address;

  connection_address = {};

  connection_address.sin_family = AF_INET;
  connection_address.sin_port = htons(0);
  connection_address.sin_addr.s_addr = htonl(INADDR_ANY);

  bind(socket_fd, (struct sockaddr *) &connection_address, sizeof(connection_address));

  socklen_t connection_len = sizeof(connection_address);

  getsockname(socket_fd, (struct sockaddr *) &connection_address, &connection_len);

  connection_port = connection_address.sin_port;
}

void Connection::serveDownload() {
  std::ifstream input_file(file_path, std::ios::binary);

  if (!input_file.is_open()) {
    return;
  }

  state = TFTPState::RECEIVED_RRQ;

  while (state != TFTPState::FINAL_ACK and state != TFTPState::ERROR) {
    std::vector<char> buffer(512);

    input_file.read(buffer.data(), 512);

    if (input_file.gcount() < 512) {
      buffer.resize(input_file.gcount());
    }

    DataPacket data_packet{blkNumber, std::vector<uint8_t>(buffer.begin(), buffer.end())};

    sendPacket(data_packet);

    std::unique_ptr<TFTPPacket> packet;
    try {
      packet = receivePacket();
    } catch (std::runtime_error &e) {
      state = TFTPState::ERROR;
      break;
    }


    auto ack_packet = dynamic_cast<ACKPacket *>(packet.get());
    auto error_packet = dynamic_cast<ErrorPacket *>(packet.get());

    if (ack_packet) {
      if (ack_packet->getBlockNumber() == blkNumber) {
        blkNumber++;
      }

      if (input_file.eof()) {
        state = TFTPState::FINAL_ACK;
      }
    } else if (error_packet) {
      state = TFTPState::ERROR;
    }
  }

  input_file.close();
}

void Connection::serveUpload() {
  std::ofstream output_file(file_path, std::ios::binary);

  if (!output_file.is_open()) {
    return;
  }

  state = TFTPState::RECEIVED_WRQ;

  while (state != TFTPState::FINAL_ACK and state != TFTPState::ERROR) {
    auto ack_packet = ACKPacket{blkNumber};
    sendPacket(ack_packet);
    blkNumber++;

    std::unique_ptr<TFTPPacket> packet;
    try {
      packet = receivePacket();
    } catch (std::runtime_error &e) {
      state = TFTPState::ERROR;
      break;
    }

    auto data_packet = dynamic_cast<DataPacket *>(packet.get());
    auto error_packet = dynamic_cast<ErrorPacket *>(packet.get());

    if (error_packet) {
      state = TFTPState::ERROR;
    } else if (!data_packet) {
      state = TFTPState::ERROR;
    }

    auto data = data_packet->getData();

    if (data_packet->getBlockNumber() == blkNumber) {
      output_file.write(reinterpret_cast<char *>(data.data()), static_cast<long>(data.size()));


      if (data_packet->getData().size() < 512) {
        state = TFTPState::FINAL_ACK;
      }
    }

  }
  auto ack_packet = ACKPacket{blkNumber};
  sendPacket(ack_packet);

  output_file.close();
}

void Connection::sendPacket(const TFTPPacket &packet) {
  std::vector<uint8_t> data = packet.serialize();


  // TODO: Error handling
  ssize_t sent = sendto(socket_fd, data.data(), data.size(), 0, (struct sockaddr *) &client_address,
                        sizeof(client_address));

}

std::unique_ptr<TFTPPacket> Connection::receivePacket() {
  std::vector<uint8_t> buffer(65535);

  socklen_t from_length = sizeof(client_address);

  ssize_t received = recvfrom(socket_fd, buffer.data(), buffer.size(), 0, (struct sockaddr *) &client_address,
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

  std::cerr << packet->formatPacket(inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port),
                                    ntohs(connection_port));
  return packet;
}
