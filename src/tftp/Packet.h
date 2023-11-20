//
// Created by Matej Sirovatka on 08.10.2023.
//

#ifndef ISA_PROJECT_PACKET_H
#define ISA_PROJECT_PACKET_H

#include <any>
#include <array>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "../utils/Options.h"
#include "common.h"

namespace TFTP {
  class Packet {
  public:
    virtual ~Packet() = default;

    [[nodiscard]] virtual std::vector<uint8_t> serialize() const = 0;

    [[nodiscard]] virtual std::string formatPacket(std::string src_ip, uint16_t port, uint16_t dst_port) const = 0;

    static std::unique_ptr<Packet> deserialize(const std::vector<uint8_t> &data);
  };

  class RRQPacket : public Packet {
    std::string mFilename;
    std::string mMode;
    Options::map_t mOptions;

  public:
    RRQPacket(std::string filename, std::string mode, Options::map_t opts) : mFilename(std::move(filename)),
                                                                             mMode(std::move(mode)),
                                                                             mOptions(std::move(opts)) {}

    [[nodiscard]] std::string formatPacket(std::string src_ip, uint16_t port, uint16_t dst_port) const override;

    [[nodiscard]] std::string getFilename() const { return mFilename; }

    [[nodiscard]] std::string getMode() const { return mMode; }

    [[nodiscard]] std::string getFormattedOptions() const { return Options::format(mOptions); }

    [[nodiscard]] Options::map_t getOptions() const { return mOptions; }

    [[nodiscard]] std::vector<uint8_t> serialize() const override;

    static std::unique_ptr<RRQPacket> deserializeFromData(const std::vector<uint8_t> &data);
  };

  class WRQPacket : public Packet {
    std::string mFilename;
    std::string mMode;
    Options::map_t mOptions;

  public:
    WRQPacket(std::string filename, std::string mode, Options::map_t opts) : mFilename(std::move(filename)),
                                                                             mMode(std::move(mode)),
                                                                             mOptions(std::move(opts)) {}

    [[nodiscard]] std::string formatPacket(std::string src_ip, uint16_t port, uint16_t dst_port) const override;

    [[nodiscard]] std::string getFilename() const { return mFilename; }

    [[nodiscard]] std::string getMode() const { return mMode; }

    [[nodiscard]] std::string getFormattedOptions() const { return Options::format(mOptions); }

    [[nodiscard]] Options::map_t getOptions() const { return mOptions; }

    [[nodiscard]] std::vector<uint8_t> serialize() const override;

    static std::unique_ptr<WRQPacket> deserializeFromData(const std::vector<uint8_t> &data);
  };

  class OACKPacket : public Packet {
    Options::map_t mOptions;

  public:
    explicit OACKPacket(Options::map_t opts) : mOptions(std::move(opts)) {}

    [[nodiscard]] std::string formatPacket(std::string src_ip, uint16_t port, uint16_t dst_port) const override;

    [[nodiscard]] Options::map_t getOptions() const { return mOptions; }

    [[nodiscard]] std::string getFormattedOptions() const { return Options::format(mOptions); }

    [[nodiscard]] std::vector<uint8_t> serialize() const override;

    static std::unique_ptr<OACKPacket> deserializeFromData(const std::vector<uint8_t> &data);
  };

  class DataPacket : public Packet {
    uint16_t mBlkNumber;
    std::vector<uint8_t> mData;

  public:
    DataPacket(uint16_t blockNumber, std::vector<uint8_t> data) : mBlkNumber(blockNumber), mData(std::move(data)) {}

    [[nodiscard]] std::string formatPacket(std::string src_ip, uint16_t port, uint16_t dst_port) const override;

    [[nodiscard]] std::vector<uint8_t> getData() const { return mData; }

    [[nodiscard]] uint16_t getBlockNumber() const { return mBlkNumber; }

    [[nodiscard]] std::vector<uint8_t> serialize() const override;

    static std::unique_ptr<DataPacket> deserializeFromData(const std::vector<uint8_t> &data);
  };

  class ACKPacket : public Packet {
    uint16_t mBlkNumber;

  public:
    explicit ACKPacket(uint16_t blockNumber) : mBlkNumber(blockNumber) {}

    [[nodiscard]] std::string formatPacket(std::string src_ip, uint16_t port, uint16_t dst_port) const override;

    [[nodiscard]] uint16_t getBlockNumber() const { return mBlkNumber; }

    [[nodiscard]] std::vector<uint8_t> serialize() const override;

    static std::unique_ptr<ACKPacket> deserializeFromData(const std::vector<uint8_t> &data);
  };

  class ErrorPacket : public Packet {
    uint16_t mErrorCode;
    std::string mErrorMessage;

  public:
    ErrorPacket(uint16_t errorCode, std::string errorMsg) : mErrorCode(errorCode), mErrorMessage(std::move(errorMsg)) {}

    [[nodiscard]] std::string formatPacket(std::string src_ip, uint16_t port, uint16_t dst_port) const override;

    [[nodiscard]] std::string getErrorCode() const { return std::to_string(mErrorCode); }

    [[nodiscard]] std::string getErrorMsg() const { return mErrorMessage; }

    [[nodiscard]] std::vector<uint8_t> serialize() const override;

    static std::unique_ptr<ErrorPacket> deserializeFromData(const std::vector<uint8_t> &data);
  };
}// namespace TFTP


#endif//ISA_PROJECT_PACKET_H
