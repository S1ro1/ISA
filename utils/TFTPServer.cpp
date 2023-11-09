//
// Created by Matej Sirovatka on 14.10.2023.
//

#include "TFTPServer.h"

TFTPServer::TFTPServer(const ServerArgs &args) {
  main_socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
  root_dir = args.root_dir;

  server_address = {};

  memset(&server_address, 0, sizeof(server_address));

  server_address.sin_family = AF_INET;
  server_address.sin_port = htons(args.port);

  // TODO: error handling
  bind(main_socket_fd, (struct sockaddr *) &server_address, sizeof(server_address));
}

void TFTPServer::listen() {
  std::vector<std::unique_ptr<Connection>> connections;
  while (true) {
    std::vector<uint8_t> buffer(65535);

    sockaddr_in from_address = {};

    socklen_t from_length = sizeof(from_address);

    ssize_t received = recvfrom(main_socket_fd, buffer.data(), buffer.size(), 0, (struct sockaddr *) &from_address,
                                &from_length);

    auto packet = TFTPPacket::deserialize(buffer);
    auto rrq_packet = dynamic_cast<RRQPacket *>(packet.get());
    auto wrq_packet = dynamic_cast<WRQPacket *>(packet.get());

    std::cerr << packet->formatPacket(inet_ntoa(from_address.sin_addr), ntohs(from_address.sin_port),
                                      ntohs(server_address.sin_port));

    // TODO: error handling
    if (rrq_packet) {
      auto file_path = root_dir + "/" + rrq_packet->getFilename();
      auto connection = std::make_unique<Connection>(file_path, rrq_packet->getOptions(),
                                                     from_address);
      connections.push_back(std::move(connection));
      threads.emplace_back(&Connection::serveDownload, connections.back().get());
    } else if (wrq_packet) {
      auto file_path = root_dir + "/" + wrq_packet->getFilename();
      auto connection = std::make_unique<Connection>(file_path, wrq_packet->getOptions(),
                                                     from_address);
      connections.push_back(std::move(connection));
      threads.emplace_back(&Connection::serveUpload, connections.back().get());
    } else {
      continue;
    }
  }
}
