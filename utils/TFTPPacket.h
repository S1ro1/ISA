//
// Created by Matej Sirovatka on 08.10.2023.
//

#ifndef ISA_PROJECT_TFTPPACKET_H
#define ISA_PROJECT_TFTPPACKET_H

#include <memory>
#include <string>
#include <utility>
#include <vector>
#include <set>
#include <array>

struct OptionsMap {
  std::pair<int, bool> mBlksize;
  std::pair<int, bool> mTimeout;
  std::pair<int, bool> mTsize;

  OptionsMap(int blksize, int timeout, int tsize) : mBlksize(std::pair{blksize, true}),
                                                    mTimeout(std::pair{timeout, true}),
                                                    mTsize(std::pair{tsize, true}) {}
  OptionsMap(const std::vector<uint8_t> &data, size_t start);

  [[nodiscard]] std::vector<uint8_t> serialize() const;

  [[nodiscard]] std::string format() const {
    std::string result;
    if (mBlksize.second) {
      result += "blksize=" + std::to_string(mBlksize.first) + " ";
    }
    if (mTimeout.second) {
      result += "timeout=" + std::to_string(mTimeout.first) + " ";
    }
    if (mTsize.second) {
      result += "tsize=" + std::to_string(mTsize.first) + " ";
    }
    return result;
  }
};

class PacketOpCodeError : public std::exception {
  std::string errorMessage;

public:
  explicit PacketOpCodeError(uint16_t op) {
    errorMessage = "Wrong opcode: " + std::to_string(op);
  }

  [[nodiscard]] const char *what() const noexcept override {
    return errorMessage.c_str();
  }
};


class PacketOptionError : public std::exception {
  std::string errorMessage;

public:
  explicit PacketOptionError(const std::string &option) {
    errorMessage = "Wrong option: " + option;
  }

  [[nodiscard]] const char *what() const noexcept override {
    return errorMessage.c_str();
  }
};


class TFTPPacket {
public:
  virtual ~TFTPPacket() = default;

  [[nodiscard]] virtual std::vector<uint8_t> serialize() const = 0;

  [[nodiscard]] virtual std::string formatPacket(std::string src_ip, uint16_t port, uint16_t dst_port) const = 0;

  static std::unique_ptr<TFTPPacket> deserialize(const std::vector<uint8_t> &data);
};


class RRQPacket : public TFTPPacket {
  std::string filename;
  std::string mode;
  OptionsMap options;

public:
  RRQPacket(std::string filename, std::string mode, OptionsMap opts) : filename(std::move(filename)),
                                                                       mode(std::move(mode)),
                                                                       options(std::move(opts)) {}

  [[nodiscard]] std::string formatPacket(std::string src_ip, uint16_t port, uint16_t dst_port) const override;

  [[nodiscard]] std::string getFilename() const { return filename; }

  [[nodiscard]] std::string getMode() const { return mode; }

  [[nodiscard]] std::string getFormattedOptions() const { return options.format(); }

  [[nodiscard]] OptionsMap getOptions() const { return options; }

  [[nodiscard]] std::vector<uint8_t> serialize() const override;

  static std::unique_ptr<RRQPacket> deserializeFromData(const std::vector<uint8_t> &data);
};


class WRQPacket : public TFTPPacket {
  std::string filename;
  std::string mode;
  OptionsMap options;

public:
  WRQPacket(std::string filename, std::string mode, OptionsMap opts) : filename(std::move(filename)),
                                                                       mode(std::move(mode)),
                                                                       options(std::move(opts)) {}

  [[nodiscard]] std::string formatPacket(std::string src_ip, uint16_t port, uint16_t dst_port) const override;

  [[nodiscard]] std::string getFilename() const { return filename; }

  [[nodiscard]] std::string getMode() const { return mode; }

  [[nodiscard]] std::string getFormattedOptions() const { return options.format(); }

  [[nodiscard]] OptionsMap getOptions() const { return options; }

  [[nodiscard]] std::vector<uint8_t> serialize() const override;

  static std::unique_ptr<WRQPacket> deserializeFromData(const std::vector<uint8_t> &data);
};


class OACKPacket : public TFTPPacket {
  OptionsMap options;

public:
  explicit OACKPacket(OptionsMap opts) : options(std::move(opts)) {}

  [[nodiscard]] std::string formatPacket(std::string src_ip, uint16_t port, uint16_t dst_port) const override;

  [[nodiscard]] OptionsMap getOptions() const { return options; }

  [[nodiscard]] std::string getFormattedOptions() const { return options.format(); }

  [[nodiscard]] std::vector<uint8_t> serialize() const override;

  static std::unique_ptr<OACKPacket> deserializeFromData(const std::vector<uint8_t> &data);
};


class DataPacket : public TFTPPacket {
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


class ACKPacket : public TFTPPacket {
  uint16_t blockNumber;

public:
  explicit ACKPacket(uint16_t blockNumber) : blockNumber(blockNumber) {}

  [[nodiscard]] std::string formatPacket(std::string src_ip, uint16_t port, uint16_t dst_port) const override;

  [[nodiscard]] uint16_t getBlockNumber() const { return blockNumber; }

  [[nodiscard]] std::vector<uint8_t> serialize() const override;

  static std::unique_ptr<ACKPacket> deserializeFromData(const std::vector<uint8_t> &data);
};


class ErrorPacket : public TFTPPacket {
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

#endif//ISA_PROJECT_TFTPPACKET_H
