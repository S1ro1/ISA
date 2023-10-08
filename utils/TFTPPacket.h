//
// Created by Matej Sirovatka on 08.10.2023.
//

#ifndef ISA_PROJECT_TFTPPACKET_H
#define ISA_PROJECT_TFTPPACKET_H

#include <utility>
#include <vector>
#include <memory>
#include <string>

class PacketOpCodeError: public std::exception {
private:
    uint16_t opcode;
    std::string errorMessage;

public:
    explicit PacketOpCodeError(uint16_t op) : opcode(op) {
        errorMessage = "Wrong opcode: " + std::to_string(op);
    }

    [[nodiscard]] const char* what() const noexcept override {
        return errorMessage.c_str();
    }
};

class TFTPPacket {
public:
    virtual ~TFTPPacket() = default;

    [[nodiscard]] virtual std::vector<uint8_t> serialize() const = 0;
    static std::unique_ptr<TFTPPacket> deserialize(const std::vector<uint8_t>& data);

};

class RRQPacket : public TFTPPacket {
    std::string filename;
    std::string mode;
    
public:

    RRQPacket(std::string filename, std::string  mode) : filename(std::move(filename)), mode(std::move(mode)) {}

    [[nodiscard]] std::string getFilename() const { return filename; }
    [[nodiscard]] std::string getMode() const { return mode; }

    [[nodiscard]] std::vector<uint8_t> serialize() const override;

    static std::unique_ptr<RRQPacket> deserializeFromData(const std::vector<uint8_t>& data);
};

class WRQPacket : public TFTPPacket {
    std::string filename;
    std::string mode;

public:
    WRQPacket(std::string filename, std::string  mode) : filename(std::move(filename)), mode(std::move(mode)) {}

    [[nodiscard]] std::string getFilename() const { return filename; }
    [[nodiscard]] std::string getMode() const { return mode; }

    [[nodiscard]] std::vector<uint8_t> serialize() const override;

    static std::unique_ptr<WRQPacket> deserializeFromData(const std::vector<uint8_t>& data);
};


class DataPacket : public TFTPPacket {
    uint16_t blockNumber;
    std::vector<uint8_t> data;

public:
    DataPacket(uint16_t blockNumber, std::vector<uint8_t> data) : blockNumber(blockNumber), data(std::move(data)) {}

    [[nodiscard]] std::string getBlockNumber() const { return std::to_string(blockNumber); }
    [[nodiscard]] std::vector<uint8_t> serialize() const override;

    static std::unique_ptr<DataPacket> deserializeFromData(const std::vector<uint8_t>& data);
};

class AckPacket : public TFTPPacket {
    uint16_t blockNumber;

public:
    explicit AckPacket(uint16_t blockNumber) : blockNumber(blockNumber) {}

    [[nodiscard]] std::string getBlockNumber() const { return std::to_string(blockNumber); }
    [[nodiscard]] std::vector<uint8_t> serialize() const override;

    static std::unique_ptr<AckPacket> deserializeFromData(const std::vector<uint8_t>& data);
};

class ErrorPacket : public TFTPPacket {
    uint16_t errorCode;
    std::string errorMsg;

public:
    ErrorPacket(uint16_t errorCode, std::string errorMsg) : errorCode(errorCode), errorMsg(std::move(errorMsg)) {}

    [[nodiscard]] std::string getErrorCode() const { return std::to_string(errorCode); }
    [[nodiscard]] std::string getErrorMsg() const { return errorMsg; }
    [[nodiscard]] std::vector<uint8_t> serialize() const override;

    static std::unique_ptr<ErrorPacket> deserializeFromData(const std::vector<uint8_t>& data);
};

#endif //ISA_PROJECT_TFTPPACKET_H
