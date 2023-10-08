//
// Created by Matej Sirovatka on 08.10.2023.
//

#include "TFTPPacket.h"

std::unique_ptr<TFTPPacket> TFTPPacket::deserialize(const std::vector<uint8_t> &data) {
    if (data.size() < 2) return nullptr;
    uint16_t opcode = (data[0] << 8) | data[1];

    switch (opcode) {
        case 1:
            return RRQPacket::deserializeFromData(data);
        case 2:
            return WRQPacket::deserializeFromData(data);
        case 3:
            return DataPacket::deserializeFromData(data);
        case 4:
            return AckPacket::deserializeFromData(data);
        case 5:
            return ErrorPacket::deserializeFromData(data);
        default:
            throw PacketOpCodeError(opcode);
    }
}

std::vector<uint8_t> RRQPacket::serialize() const {
    std::vector<uint8_t> output;

    output.push_back(0);
    output.push_back(1);
    for (char c : filename) {
        output.push_back(static_cast<uint8_t>(c));
    }

    output.push_back(0);

    for (char c : mode) {
        output.push_back(static_cast<uint8_t>(c));
    }

    output.push_back(0);

    return output;
}

std::unique_ptr<RRQPacket> RRQPacket::deserializeFromData(const std::vector<uint8_t> &data) {
    return std::unique_ptr<RRQPacket>();
}

std::vector<uint8_t> WRQPacket::serialize() const {
    std::vector<uint8_t> output;

    output.push_back(0);
    output.push_back(2);
    for (char c : filename) {
        output.push_back(static_cast<uint8_t>(c));
    }

    output.push_back(0);

    for (char c : mode) {
        output.push_back(static_cast<uint8_t>(c));
    }

    output.push_back(0);

    return output;
}

std::unique_ptr<WRQPacket> WRQPacket::deserializeFromData(const std::vector<uint8_t> &data) {
    return std::unique_ptr<WRQPacket>();
}

std::vector<uint8_t> DataPacket::serialize() const {
    std::vector<uint8_t> output;

    output.push_back(0);
    output.push_back(3);

    output.push_back((blockNumber >> 8) & 0xFF);
    output.push_back(blockNumber & 0xFF);

    for (uint8_t byte: data) {
        output.push_back(byte);
    }

    return output;
}

std::unique_ptr<DataPacket> DataPacket::deserializeFromData(const std::vector<uint8_t> &data) {
    return std::unique_ptr<DataPacket>();
}

std::vector<uint8_t> AckPacket::serialize() const {
    std::vector<uint8_t> output;

    output.push_back(0);
    output.push_back(4);

    output.push_back((blockNumber >> 8) & 0xFF);
    output.push_back(blockNumber & 0xFF);

    return output;
}

std::unique_ptr<AckPacket> AckPacket::deserializeFromData(const std::vector<uint8_t> &data) {
    return std::unique_ptr<AckPacket>();
}

std::vector<uint8_t> ErrorPacket::serialize() const {
    std::vector<uint8_t> output;

    output.push_back(0);
    output.push_back(5);

    output.push_back((errorCode >> 8) & 0xFF);
    output.push_back(errorCode & 0xFF);

    for (char c : errorMsg) {
        output.push_back(static_cast<uint8_t>(c));
    }

    output.push_back(0);

    return output;
}

std::unique_ptr<ErrorPacket> ErrorPacket::deserializeFromData(const std::vector<uint8_t> &data) {
    return std::unique_ptr<ErrorPacket>();
}
