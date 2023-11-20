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
    std::string filename;
    std::string mode;
    Options::map_t options;

  public:
    RRQPacket(std::string filename, std::string mode, Options::map_t opts) : filename(std::move(filename)),
                                                                             mode(std::move(mode)),
                                                                             options(std::move(opts)) {}

    [[nodiscard]] std::string formatPacket(std::string src_ip, uint16_t port, uint16_t dst_port) const override;

    [[nodiscard]] std::string getFilename() const { return filename; }

    [[nodiscard]] std::string getMode() const { return mode; }

    [[nodiscard]] std::string getFormattedOptions() const { return Options::format(options); }

    [[nodiscard]] Options::map_t getOptions() const { return options; }

    [[nodiscard]] std::vector<uint8_t> serialize() const override;

    static std::unique_ptr<RRQPacket> deserializeFromData(const std::vector<uint8_t> &data);
  };
  class WRQPacket : public Packet {
    std::string filename;
    std::string mode;
    Options::map_t options;

  public:
    WRQPacket(std::string filename, std::string mode, Options::map_t opts) : filename(std::move(filename)),
                                                                             mode(std::move(mode)),
                                                                             options(std::move(opts)) {}

    [[nodiscard]] std::string formatPacket(std::string src_ip, uint16_t port, uint16_t dst_port) const override;

    [[nodiscard]] std::string getFilename() const { return filename; }

    [[nodiscard]] std::string getMode() const { return mode; }

    [[nodiscard]] std::string getFormattedOptions() const { return Options::format(options); }

    [[nodiscard]] Options::map_t getOptions() const { return options; }

    [[nodiscard]] std::vector<uint8_t> serialize() const override;

    static std::unique_ptr<WRQPacket> deserializeFromData(const std::vector<uint8_t> &data);
  };
  class OACKPacket : public Packet {
    Options::map_t options;

  public:
    explicit OACKPacket(Options::map_t opts) : options(std::move(opts)) {}

    [[nodiscard]] std::string formatPacket(std::string src_ip, uint16_t port, uint16_t dst_port) const override;

    [[nodiscard]] Options::map_t getOptions() const { return options; }

    [[nodiscard]] std::string getFormattedOptions() const { return Options::format(options); }

    [[nodiscard]] std::vector<uint8_t> serialize() const override;

    static std::unique_ptr<OACKPacket> deserializeFromData(const std::vector<uint8_t> &data);
  };
  class DataPacket : public Packet {
    uint16_t blockNumber;
    std::vector<uint8_t> data;

  public:
    DataPacket(uint16_t blockNumber, std::vector<uint8_t> data) : blockNumber(blockNumber), data(std::move(data)) {}

    [[nodiscard]] std::string formatPacket(std::string src_ip, uint16_t port, uint16_t dst_port) const override;

    [[nodiscard]] std::vector<uint8_t> getData() const { return data; }

    [[nodiscard]] uint16_t getBlockNumber() const { return blockNumber; }

    [[nodiscard]] std::vector<uint8_t> serialize() const override;

    static std::unique_ptr<DataPacket> deserializeFromData(const std::vector<uint8_t> &data);
  };
  class ACKPacket : public Packet {
    uint16_t blockNumber;

  public:
    explicit ACKPacket(uint16_t blockNumber) : blockNumber(blockNumber) {}

    [[nodiscard]] std::string formatPacket(std::string src_ip, uint16_t port, uint16_t dst_port) const override;

    [[nodiscard]] uint16_t getBlockNumber() const { return blockNumber; }

    [[nodiscard]] std::vector<uint8_t> serialize() const override;

    static std::unique_ptr<ACKPacket> deserializeFromData(const std::vector<uint8_t> &data);
  };
  class ErrorPacket : public Packet {
    uint16_t errorCode;
    std::string errorMsg;

  public:
    ErrorPacket(uint16_t errorCode, std::string errorMsg) : errorCode(errorCode), errorMsg(std::move(errorMsg)) {}

    [[nodiscard]] std::string formatPacket(std::string src_ip, uint16_t port, uint16_t dst_port) const override;

    [[nodiscard]] std::string getErrorCode() const { return std::to_string(errorCode); }

    [[nodiscard]] std::string getErrorMsg() const { return errorMsg; }

    [[nodiscard]] std::vector<uint8_t> serialize() const override;

    static std::unique_ptr<ErrorPacket> deserializeFromData(const std::vector<uint8_t> &data);
  };
}// namespace TFTP


#endif//ISA_PROJECT_PACKET_H