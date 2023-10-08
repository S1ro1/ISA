//
// Created by Matej Sirovatka on 08.10.2023.
//

#include "TFTPPacket.h"

std::unique_ptr<TFTPPacket> TFTPPacket::deserialize(const std::vector<uint8_t> &data) {
    return std::unique_ptr<TFTPPacket>();
}

std::vector<uint8_t> RRQPacket::serialize() const {
    return std::vector<uint8_t>();
}

std::unique_ptr<RRQPacket> RRQPacket::deserializeFromData(const std::vector<uint8_t> &data) {
    return std::unique_ptr<RRQPacket>();
}

std::vector<uint8_t> WRQPacket::serialize() const {
    return std::vector<uint8_t>();
}

std::unique_ptr<WRQPacket> WRQPacket::deserializeFromData(const std::vector<uint8_t> &data) {
    return std::unique_ptr<WRQPacket>();
}

std::vector<uint8_t> DataPacket::serialize() const {
    return std::vector<uint8_t>();
}

std::unique_ptr<DataPacket> DataPacket::deserializeFromData(const std::vector<uint8_t> &data) {
    return std::unique_ptr<DataPacket>();
}

std::vector<uint8_t> AckPacket::serialize() const {
    return std::vector<uint8_t>();
}

std::unique_ptr<AckPacket> AckPacket::deserializeFromData(const std::vector<uint8_t> &data) {
    return std::unique_ptr<AckPacket>();
}

std::vector<uint8_t> ErrorPacket::serialize() const {
    return std::vector<uint8_t>();
}

std::unique_ptr<ErrorPacket> ErrorPacket::deserializeFromData(const std::vector<uint8_t> &data) {
    return std::unique_ptr<ErrorPacket>();
}
