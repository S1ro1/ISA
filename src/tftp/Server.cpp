//
// Created by Matej Sirovatka on 14.10.2023.
//

#include "Server.h"

volatile sig_atomic_t runningServer = 1;

void ServerSigintHandler(int signum) {
  runningServer = 0;
}

TFTP::Server::Server(const ServerArgs &args) {
  main_socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
  root_dir = args.root_dir;

  struct sigaction sa;
  sa.sa_handler = ServerSigintHandler;

  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;
  sigaction(SIGINT, &sa, NULL);

  // remove SA_RESTART from the SIGINT handler
  auto val = sigaction(SIGINT, &sa, NULL);

  server_address = {};
  memset(&server_address, 0, sizeof(server_address));

  server_address.sin_family = AF_INET;
  server_address.sin_port = htons(args.port);

//  timeval read_timeout{};
//  read_timeout.tv_sec = 0;
//  read_timeout.tv_usec = 100;
//  setsockopt(main_socket_fd, SOL_SOCKET, SO_RCVTIMEO, &read_timeout, sizeof read_timeout);

  bind(main_socket_fd, reinterpret_cast<sockaddr *>(&server_address), sizeof(server_address));
}

void TFTP::Server::listen() {
  while (runningServer) {
    std::vector<uint8_t> buffer(65535);

    sockaddr_in from_address = {};
    socklen_t from_length = sizeof(from_address);

    ssize_t received = recvfrom(main_socket_fd, buffer.data(), buffer.size(), 0, (struct sockaddr *) &from_address,
                                &from_length);

    if (received <= 0) {
      if (errno == EINTR) {
        break;
      } else {
        continue;
      }
    }
    buffer.resize(received);

    std::unique_ptr<Packet> packet;
    try {
      packet = Packet::deserialize(buffer);
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
    std::filesystem::path path{root_dir};
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
      auto connection = std::make_unique<TFTP::Connection>(path, validated_options,
                                                           from_address, rrq_packet->getMode());
      connections.push_back(std::move(connection));
      threads.emplace_back(&TFTP::Connection::serveDownload, connections.back().get());

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
      auto connection = std::make_unique<TFTP::Connection>(path, validated_options,
                                                           from_address, wrq_packet->getMode());
      connections.push_back(std::move(connection));
      threads.emplace_back(&TFTP::Connection::serveUpload, connections.back().get());

    } else {
      auto error_packet = ErrorPacket{4, "Illegal TFTP operation"}.serialize();
      sendto(main_socket_fd, error_packet.data(), error_packet.size(), 0, (struct sockaddr *) &from_address,
             sizeof(from_address));
    }
  }
}

TFTP::Server::~Server() {
  for (auto &connection: connections) {
    connection->cleanup();
  }

  for (auto &thread: threads) {
    thread.join();
  }
}
