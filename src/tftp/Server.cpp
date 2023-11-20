//
// Created by Matej Sirovatka on 14.10.2023.
//

#include "Server.h"

volatile sig_atomic_t runningServer = 1;

void ServerSigintHandler(int signum) {
  runningServer = 0;
}

TFTP::Server::Server(const ServerArgs &args) {
  mMainSocketFd = socket(AF_INET, SOCK_DGRAM, 0);
  mRootDir = args.mRootDir;

  struct sigaction sa;
  sa.sa_handler = ServerSigintHandler;

  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;
  sigaction(SIGINT, &sa, NULL);

  mServerAdress = {};
  memset(&mServerAdress, 0, sizeof(mServerAdress));

  mServerAdress.sin_family = AF_INET;
  mServerAdress.sin_port = htons(args.mPort);

  //  timeval read_timeout{};
  //  read_timeout.tv_sec = 0;
  //  read_timeout.tv_usec = 100;
  //  setsockopt(mMainSocketFd, SOL_SOCKET, SO_RCVTIMEO, &read_timeout, sizeof read_timeout);

  bind(mMainSocketFd, reinterpret_cast<sockaddr *>(&mServerAdress), sizeof(mServerAdress));
}

void TFTP::Server::listen() {
  while (runningServer) {
    std::vector<uint8_t> buffer(65535);

    sockaddr_in from_address = {};
    socklen_t from_length = sizeof(from_address);

    ssize_t received = recvfrom(mMainSocketFd, buffer.data(), buffer.size(), 0, (struct sockaddr *) &from_address,
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
      sendto(mMainSocketFd, error_packet.data(), error_packet.size(), 0, (struct sockaddr *) &from_address,
             sizeof(from_address));

      continue;
    }
    const auto rrq_packet = dynamic_cast<RRQPacket *>(packet.get());
    const auto wrq_packet = dynamic_cast<WRQPacket *>(packet.get());

    std::cerr << packet->formatPacket(inet_ntoa(from_address.sin_addr), ntohs(from_address.sin_port),
                                      ntohs(mServerAdress.sin_port));

    // TODO: error handling
    std::filesystem::path path{mRootDir};
    Options::map_t validated_options;
    if (rrq_packet) {
      path /= rrq_packet->getFilename();
      try {
        validated_options = Options::validate(rrq_packet->getOptions());
      } catch (Options::InvalidFormatException &e) {
        auto error_packet = ErrorPacket{4, "Illegal TFTP operation"}.serialize();
        sendto(mMainSocketFd, error_packet.data(), error_packet.size(), 0, (struct sockaddr *) &from_address,
               sizeof(from_address));
        continue;
      }
      auto connection = std::make_unique<TFTP::Connection>(path, validated_options,
                                                           from_address, rrq_packet->getMode());
      mConnections.push_back(std::move(connection));
      mThreads.emplace_back(&TFTP::Connection::serveDownload, mConnections.back().get());

    } else if (wrq_packet) {
      path /= wrq_packet->getFilename();
      try {
        validated_options = Options::validate(wrq_packet->getOptions());
      } catch (Options::InvalidFormatException &e) {
        auto error_packet = ErrorPacket{4, "Illegal TFTP operation"}.serialize();
        sendto(mMainSocketFd, error_packet.data(), error_packet.size(), 0, (struct sockaddr *) &from_address,
               sizeof(from_address));
        continue;
      }
      auto connection = std::make_unique<TFTP::Connection>(path, validated_options,
                                                           from_address, wrq_packet->getMode());
      mConnections.push_back(std::move(connection));
      mThreads.emplace_back(&TFTP::Connection::serveUpload, mConnections.back().get());

    } else {
      auto error_packet = ErrorPacket{4, "Illegal TFTP operation"}.serialize();
      sendto(mMainSocketFd, error_packet.data(), error_packet.size(), 0, (struct sockaddr *) &from_address,
             sizeof(from_address));
    }
  }
}

TFTP::Server::~Server() {
  for (auto &connection: mConnections) {
    connection->cleanup();
  }

  for (std::thread &thread: mThreads) {
    pthread_kill(thread.native_handle(), SIGUSR1);
  }

  for (auto &thread: mThreads) {
    thread.join();
  }
}
