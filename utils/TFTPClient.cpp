//
// Created by Matej Sirovatka on 09.10.2023.
//

#include "TFTPClient.h"

void TFTPClient::transmit() {
    if (mode == Mode::DOWNLOAD) {
        requestRead();
    }
}

TFTPClient::TFTPClient(const ClientArgs &args) {
    socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    dst_file_path = args.dst_file_path;
    state = TFTPState::INIT;

    if (args.src_file_path.has_value()) {
        mode = Mode::DOWNLOAD;
        src_file_path = args.src_file_path.value();
    } else mode = Mode::UPLOAD;

    // TODO: error handling

    memset(&server_address, 0, sizeof(server_address));

    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(args.port);

    if (inet_pton(AF_INET, args.address.c_str(), &server_address.sin_addr) <= 0) {
        exit(1);
    }
}

void TFTPClient::sendPacket(const TFTPPacket &packet) {
    std::vector<uint8_t> data = packet.serialize();

    ssize_t sent = sendto(socket_fd, data.data(), data.size(), 0, (struct sockaddr *) &server_address,
                          sizeof(server_address));

    // TODO: error handling
}

std::unique_ptr<TFTPPacket> TFTPClient::receivePacket() {
    std::vector<uint8_t> buffer(1024);

    sockaddr_in from_address = {};

    socklen_t from_length = sizeof(from_address);

    ssize_t received = recvfrom(socket_fd, buffer.data(), buffer.size(), 0, (struct sockaddr *) &from_address,
                                &from_length);

    //TODO: error handling

    if (state == TFTPState::SENT_RRQ || state == TFTPState::SENT_WRQ) server_address.sin_port = from_address.sin_port;

    return TFTPPacket::deserialize(buffer);
}

void TFTPClient::requestRead() {
    std::ofstream outputFile(dst_file_path, std::ios::binary);

    // TODO: error handling

    RRQPacket rrq(src_file_path, transmissionMode, opts);
    sendPacket(rrq);

    state = TFTPState::SENT_RRQ;

    while (state != TFTPState::FINAL_ACK && state != TFTPState::ERROR) {
        auto packet = receivePacket();

        auto data_packet = dynamic_cast<DataPacket *>(packet.get());
        if (data_packet) {
            uint16_t blockNumber = std::strtol(data_packet->getBlockNumber().c_str(), nullptr, 10);
            AckPacket ack(blockNumber);
            sendPacket(ack);

            auto data = data_packet->getData();
            if (data.size() < 512) {
                state = TFTPState::FINAL_ACK;
            }
            outputFile.write(reinterpret_cast<char *>(data.data()), static_cast<long>(data.size()));

            continue;
        }

        auto oack_packet = dynamic_cast<OACKPacket *>(packet.get());

        if (oack_packet) {
            //TODO: validate options

            opts = oack_packet->getOptions();
            continue;
        }

        auto err_packet = dynamic_cast<ErrorPacket *>(packet.get());

        if (err_packet) {
            // TODO: error handling
            continue;
        }
    }

    outputFile.close();
}

