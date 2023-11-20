//
// Created by Matej Sirovatka on 14.10.2023.
//

#include "TFTPServer.h"
#include "TFTP.h"

volatile sig_atomic_t running = 1;

void siginthandler([[maybe_unused]] int signum) {
  running = 0;
}

TFTPServer::TFTPServer(const ServerArgs &args) {
  signal(SIGINT, siginthandler);
  main_socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
  root_dir = args.root_dir;

  server_address = {};

  memset(&server_address, 0, sizeof(server_address));

  server_address.sin_family = AF_INET;
  server_address.sin_port = htons(args.port);

  timeval read_timeout{};
  read_timeout.tv_sec = 0;
  read_timeout.tv_usec = 100;
  setsockopt(main_socket_fd, SOL_SOCKET, SO_RCVTIMEO, &read_timeout, sizeof read_timeout);

  // TODO: error handling
  bind(main_socket_fd, reinterpret_cast<sockaddr *>(&server_address), sizeof(server_address));
}

void TFTPServer::listen() {
  while (running) {
    std::vector<uint8_t> buffer(65535);

    sockaddr_in from_address = {};
    socklen_t from_length = sizeof(from_address);

    ssize_t received = recvfrom(main_socket_fd, buffer.data(), buffer.size(), 0, (struct sockaddr *) &from_address,
                                &from_length);

    if (received <= 0) {
      continue;
    }
    buffer.resize(received);

    std::unique_ptr<TFTPPacket> packet;
    try {
      packet = TFTPPacket::deserialize(buffer);
    } catch (TFTP::PacketFormatException &e) {
      auto error_packet = ErrorPacket{4, "Illegal TFTP operation"}.serialize();
      sendto(main_socket_fd, error_packet.data(), error_packet.size(), 0, (struct sockaddr *) &from_address,
                            sizeof(from_address));

      continue;
    }
    const auto rrq_packet = dynamic_cast<RRQPacket *>(packet.get());
    const auto wrq_packet = dynamic_cast<WRQPacket *>(packet.get());

    std::cerr << packet->formatPacket(inet_ntoa(from_address.sin_addr), ntohs(from_address.sin_port),
                                      ntohs(server_address.sin_port));

    // TODO: error handling
    std::filesystem::path path {root_dir};
    Options::map_t validated_options;
    if (rrq_packet) {
      path /= rrq_packet->getFilename();
      try {
        validated_options = Options::validate(rrq_packet->getOptions());
      } catch (Options::InvalidFormatException &e) {
        auto error_packet = ErrorPacket{4, "Illegal TFTP operation"}.serialize();
        sendto(main_socket_fd, error_packet.data(), error_packet.size(), 0, (struct sockaddr *) &from_address,
                              sizeof(from_address));
        continue;
      }
      auto connection = std::make_unique<Connection>(path, validated_options,
                                                     from_address, rrq_packet->getMode());
      connections.push_back(std::move(connection));
      threads.emplace_back(&Connection::serveDownload, connections.back().get());

    } else if (wrq_packet) {
      path /= wrq_packet->getFilename();
      try {
        validated_options = Options::validate(wrq_packet->getOptions());
      } catch (Options::InvalidFormatException &e) {
        auto error_packet = ErrorPacket{4, "Illegal TFTP operation"}.serialize();
        sendto(main_socket_fd, error_packet.data(), error_packet.size(), 0, (struct sockaddr *) &from_address,
               sizeof(from_address));
        continue;
      }
      auto connection = std::make_unique<Connection>(path, validated_options,
                                                     from_address, wrq_packet->getMode());
      connections.push_back(std::move(connection));
      threads.emplace_back(&Connection::serveUpload, connections.back().get());

    } else {
      auto error_packet = ErrorPacket{4, "Illegal TFTP operation"}.serialize();
      sendto(main_socket_fd, error_packet.data(), error_packet.size(), 0, (struct sockaddr *) &from_address,
             sizeof(from_address));
    }
  }
}

TFTPServer::~TFTPServer() {
  for (auto &connection : connections) {
    connection->cleanup();
  }

  for (auto &thread : threads) {
    thread.join();
  }
}
