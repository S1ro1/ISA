//
// Created by Matej Sirovatka on 14.10.2023.
//

#include "Connection.h"

#include <utility>

Connection::Connection(std::string file, OptionsMap options, sockaddr_in client_address) {
    this->file_path = std::move(file);
    this->opts = std::move(options);

    state = TFTPState::INIT;
    socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    this->client_address = client_address;

    address = {};

    memset(&address, 0, sizeof(address));

    address.sin_family = AF_INET;
    address.sin_port = htons(0);
    address.sin_addr.s_addr = htonl(INADDR_ANY);

    // TODO: Error handling
    bind(socket_fd, (struct sockaddr *) &address, sizeof(address));
}

void Connection::serveDownload() {
    std::ifstream input_file(file_path, std::ios::binary);

    if (!input_file.is_open()) {
        std::cout << "File not found" << std::endl;
        return;
    }

    state = TFTPState::RECEIVED_RRQ;

    while (state != TFTPState::FINAL_ACK and state != TFTPState::ERROR) {
        std::vector<char> buffer(512);
        std::cout << "Reading..." << std::endl;

        input_file.read(buffer.data(), 512);

        if (input_file.gcount() < 512) {
            buffer.resize(input_file.gcount());
        }

        DataPacket data_packet{blkNumber, std::vector<uint8_t>(buffer.begin(), buffer.end())};

        sendPacket(data_packet);

        auto packet = receivePacket();

        std::cout << "Received packet..." << std::endl;

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
        std::cout << "File not found" << std::endl;
        return;
    }

    std::cout << file_path << std::endl;

    state = TFTPState::RECEIVED_WRQ;
    auto ack_packet = ACKPacket{blkNumber};
    sendPacket(ack_packet);
    blkNumber++;

    while (state != TFTPState::FINAL_ACK and state != TFTPState::ERROR) {

        auto packet = receivePacket();

        auto data_packet = dynamic_cast<DataPacket *>(packet.get());
        auto error_packet = dynamic_cast<ErrorPacket *>(packet.get());

        if (error_packet) {
            std::cout << error_packet->getErrorMsg() << std::endl;
            state = TFTPState::ERROR;
        } else if (!data_packet) {
            std::cout << "Invalid packet received" << std::endl;
            state = TFTPState::ERROR;
        }

        auto data = data_packet->getData();

        std::cout << data_packet->getBlockNumber() << std::endl;

        if (data_packet->getBlockNumber() == blkNumber) {
            std::cout << reinterpret_cast<char *>(data.data()) << std::endl;
            output_file.write(reinterpret_cast<char *>(data.data()), static_cast<long>(data.size()));

            blkNumber++;

            if (data_packet->getData().size() < 512) {
                std::cout << "Final block" << std::endl;
                state = TFTPState::FINAL_ACK;
            }
        }

        ack_packet = ACKPacket{blkNumber};
        sendPacket(ack_packet);
    }

    std::cout << "Closing file" << std::endl;

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

    buffer.resize(received);

    return TFTPPacket::deserialize(buffer);
}
