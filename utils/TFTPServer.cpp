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

        // TODO: error handling
        if (rrq_packet) {
            auto file_path = root_dir + "/" + rrq_packet->getFilename();
            auto connection = std::make_unique<Connection>(file_path, Mode::DOWNLOAD, rrq_packet->getOptions(),
                                                           from_address);

            connections.push_back(std::move(connection));

            std::jthread thread([conn = connections.back().get()]() {
                conn->serveDownload();
            });
            threads.push_back(std::move(thread));
        } else if (wrq_packet) {
            auto file_path = root_dir + "/" + wrq_packet->getFilename();
            auto connection = std::make_unique<Connection>(file_path, Mode::DOWNLOAD, rrq_packet->getOptions(),
                                                           from_address);

            connections.push_back(std::move(connection));

            std::jthread thread([conn = connections.back().get()]() {
                conn->serveDownload();
            });
            threads.push_back(std::move(thread));
        } else {
            std::cout << "err" << std::endl;
            continue;
        }
    }
}
