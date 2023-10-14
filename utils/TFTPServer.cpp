//
// Created by Matej Sirovatka on 14.10.2023.
//

#include "TFTPServer.h"

TFTPServer::TFTPServer(const ServerArgs &args) {
    main_socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    root_dir = args.root_dir;

    memset(&server_address, 0, sizeof(server_address));

    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(args.port);

    // TODO: error handling
    bind(main_socket_fd, (struct sockaddr *) &server_address, sizeof(server_address));
}

void TFTPServer::listen() {
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
            std::cout << "RRQ" << std::endl;
        } else if (wrq_packet) {
            std::cout << "WRQ" << std::endl;
        } else {
            continue;
        }
    }

}
