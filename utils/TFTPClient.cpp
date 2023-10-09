//
// Created by Matej Sirovatka on 09.10.2023.
//

#include "TFTPClient.h"

void TFTPClient::transmit() {
    if (mode == Mode::DOWNLOAD) {
        requestRead();
    } else {
        requestWrite();
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

void TFTPClient::handleDataPacket(std::ofstream &outputFile, DataPacket *data_packet) {
    uint16_t blockNumber = std::strtol(data_packet->getBlockNumber().c_str(), nullptr, 10);
    AckPacket ack(blockNumber);
    sendPacket(ack);

    auto data = data_packet->getData();
    if (data.size() < 512) {
        state = TFTPState::FINAL_ACK;
    }
    outputFile.write(reinterpret_cast<char *>(data.data()), static_cast<long>(data.size()));
}

void TFTPClient::requestRead() {
    std::ofstream outputFile(dst_file_path, std::ios::binary);

    // TODO: error handling

    RRQPacket rrq(src_file_path, transmissionMode, opts);
    sendPacket(rrq);

    state = TFTPState::SENT_RRQ;

    auto packet = receivePacket();

    auto oack_packet = dynamic_cast<OACKPacket *>(packet.get());
    auto data_packet = dynamic_cast<DataPacket *>(packet.get());

    if (oack_packet) {
        opts = oack_packet->getOptions();
    } else if (data_packet) {
        opts = OptionsMap{};
        handleDataPacket(outputFile, data_packet);
    } else {
        state = TFTPState::ERROR;
        outputFile.close();
        return;
    }

    while (state != TFTPState::FINAL_ACK && state != TFTPState::ERROR) {
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
    auto ack_packet = dynamic_cast<AckPacket *>(packet.get());

    if (oack_packet) {
        opts = oack_packet->getOptions();
    } else if (ack_packet) {
    } else {
        state = TFTPState::ERROR;
        return;
    }

    uint16_t blockNumber = 1;

    while (state != TFTPState::FINAL_ACK && state != TFTPState::ERROR) {
        // TODO: check with options
        std::vector<uint8_t> data(512);

        std::cin.read(reinterpret_cast<char *>(data.data()), static_cast<long>(data.size()));
        size_t bytesRead = std::cin.gcount();

        if (bytesRead < data.size()) data.resize(bytesRead);

        DataPacket dataPacket(blockNumber, data);
        sendPacket(dataPacket);

        packet = receivePacket();

        auto ack_packet = dynamic_cast<AckPacket *>(packet.get());

        if (!ack_packet) {
            state = TFTPState::ERROR;
            return;
        }
        blockNumber++;

        if (bytesRead < 512) state = TFTPState::FINAL_ACK;
    }
}

