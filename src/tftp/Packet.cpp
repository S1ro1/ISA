// Matej Sirovatka, xsirov00

#include "Packet.h"

std::unique_ptr<TFTP::Packet> TFTP::Packet::deserialize(const std::vector<uint8_t> &data) {
  if (data.size() < 2) throw TFTP::PacketFormatException();
  uint16_t opcode = (data[0] << 8) | data[1];

  try {
    switch (opcode) {
      case 1:
        return RRQPacket::deserializeFromData(data);
      case 2:
        return WRQPacket::deserializeFromData(data);
      case 3:
        return TFTP::DataPacket::deserializeFromData(data);
      case 4:
        return TFTP::ACKPacket::deserializeFromData(data);
      case 5:
        return TFTP::ErrorPacket::deserializeFromData(data);
      case 6:
        return TFTP::OACKPacket::deserializeFromData(data);
      default:
        throw TFTP::PacketFormatException();
    }
  } catch (TFTP::PacketFormatException &e) {
    throw e;
  }
}

std::vector<uint8_t> TFTP::RRQPacket::serialize() const {
  std::vector<uint8_t> output;

  output.push_back(0);
  output.push_back(1);
  for (char c: mFilename) {
    output.push_back(static_cast<uint8_t>(c));
  }

  output.push_back(0);

  for (char c: mMode) {
    output.push_back(static_cast<uint8_t>(c));
  }

  output.push_back(0);
  auto opts = Options::serialize(mOptions);
  output.insert(output.end(), opts.begin(), opts.end());
  return output;
}

std::unique_ptr<TFTP::RRQPacket> TFTP::RRQPacket::deserializeFromData(const std::vector<uint8_t> &data) {
  int idx = 2;

  std::string fname;
  while (data[idx] && idx < data.size()) {
    fname += static_cast<char>(data[idx++]);
  }

  if (idx == data.size()) throw TFTP::PacketFormatException();
  if (data[idx] != 0) throw TFTP::PacketFormatException();

  idx++;
  std::string mode;
  while (data[idx] && idx < data.size()) {
    mode += static_cast<char>(data[idx++]);
  }
  if (idx == data.size()) throw TFTP::PacketFormatException();
  if (data[idx] != 0) throw TFTP::PacketFormatException();

  idx++;
  Options::map_t opts;
  try {
    opts = Options::parse(data, idx);
  } catch (Options::InvalidFormatException &e) {
    throw TFTP::PacketFormatException();
  }
  return std::make_unique<RRQPacket>(fname, mode, opts);
}

std::string TFTP::RRQPacket::formatPacket(std::string src_ip, uint16_t port, uint16_t dst_port) const {
  std::string result = "RRQ " + src_ip + ":" + std::to_string(port) + " " + "\"" + mFilename + "\" " + mMode;
  if (!mOptions.empty()) {
    result += " " + Options::format(mOptions);
  }
  result += "\n";
  return result;
}

std::vector<uint8_t> TFTP::WRQPacket::serialize() const {
  std::vector<uint8_t> output;
  output.push_back(0);
  output.push_back(2);
  for (char c: mFilename) {
    output.push_back(static_cast<uint8_t>(c));
  }

  output.push_back(0);

  for (char c: mMode) {
    output.push_back(static_cast<uint8_t>(c));
  }

  output.push_back(0);
  auto opts = Options::serialize(mOptions);
  output.insert(output.end(), opts.begin(), opts.end());
  return output;
}

std::unique_ptr<TFTP::WRQPacket> TFTP::WRQPacket::deserializeFromData(const std::vector<uint8_t> &data) {
  int idx = 2;
  std::string fname;
  while (data[idx] && idx < data.size()) {
    fname += static_cast<char>(data[idx++]);
  }

  if (idx == data.size()) throw TFTP::PacketFormatException();
  if (data[idx] != 0) throw TFTP::PacketFormatException();

  idx++;
  std::string mode;
  while (data[idx] && idx < data.size()) {
    mode += static_cast<char>(data[idx++]);
  }
  if (idx == data.size()) throw TFTP::PacketFormatException();
  if (data[idx] != 0) throw TFTP::PacketFormatException();
  idx++;

  Options::map_t opts;
  try {
    opts = Options::parse(data, idx);
  } catch (Options::InvalidFormatException &e) {
    throw TFTP::PacketFormatException();
  }
  return std::make_unique<WRQPacket>(fname, mode, opts);
}

std::string TFTP::WRQPacket::formatPacket(std::string src_ip, uint16_t port, uint16_t dst_port) const {
  std::string result = "WRQ " + src_ip + ":" + std::to_string(port) + " " + "\"" + mFilename + "\" " + mMode;
  if (!mOptions.empty()) {
    result += " " + Options::format(mOptions);
  }
  result += "\n";
  return result;
}

std::vector<uint8_t> TFTP::DataPacket::serialize() const {
  std::vector<uint8_t> output;

  output.push_back(0);
  output.push_back(3);

  output.push_back((mBlkNumber >> 8) & 0xFF);
  output.push_back(mBlkNumber & 0xFF);

  for (uint8_t byte: mData) {
    output.push_back(byte);
  }

  return output;
}

std::unique_ptr<TFTP::DataPacket> TFTP::DataPacket::deserializeFromData(const std::vector<uint8_t> &data) {
  if (data.size() < 4) {
    throw TFTP::PacketFormatException();
  }
  uint16_t blockNum = (data[2] << 8) | data[3];
  std::vector<uint8_t> outData(data.begin() + 4, data.end());
  return std::make_unique<DataPacket>(blockNum, outData);
}

std::string TFTP::DataPacket::formatPacket(std::string src_ip, uint16_t port, uint16_t dst_port) const {
  std::string result = "DATA " + src_ip + ":" + std::to_string(port) + ":" + std::to_string(dst_port) + " " +
                       std::to_string(mBlkNumber) + "\n";

  return result;
}

std::vector<uint8_t> TFTP::ACKPacket::serialize() const {
  std::vector<uint8_t> output;
  output.push_back(0);
  output.push_back(4);
  output.push_back((mBlkNumber >> 8) & 0xFF);
  output.push_back(mBlkNumber & 0xFF);
  return output;
}

std::unique_ptr<TFTP::ACKPacket> TFTP::ACKPacket::deserializeFromData(const std::vector<uint8_t> &data) {
  if (data.size() != 4) {
    throw TFTP::PacketFormatException();
  }

  uint16_t blockNum = (data[2] << 8) | data[3];
  return std::make_unique<ACKPacket>(blockNum);
}

std::string TFTP::ACKPacket::formatPacket(std::string src_ip, uint16_t port, uint16_t dst_port) const {
  std::string result = "ACK " + src_ip + ":" + std::to_string(port) + " " + std::to_string(mBlkNumber) + "\n";
  return result;
}

std::vector<uint8_t> TFTP::ErrorPacket::serialize() const {
  std::vector<uint8_t> output;
  output.push_back(0);
  output.push_back(5);

  output.push_back((mErrorCode >> 8) & 0xFF);
  output.push_back(mErrorCode & 0xFF);

  for (char c: mErrorMessage) {
    output.push_back(static_cast<uint8_t>(c));
  }

  output.push_back(0);
  return output;
}

std::unique_ptr<TFTP::ErrorPacket> TFTP::ErrorPacket::deserializeFromData(const std::vector<uint8_t> &data) {
  if (data.size() < 4) {
    throw TFTP::PacketFormatException();
  }

  uint16_t errorCode = (data[2] << 8) | data[3];

  int idx = 4;
  std::string errorMessage;
  while (data[idx] && idx < data.size()) {
    errorMessage += static_cast<char>(data[idx++]);
  }

  return std::make_unique<ErrorPacket>(errorCode, errorMessage);
}

std::string TFTP::ErrorPacket::formatPacket(std::string src_ip, uint16_t port, uint16_t dst_port) const {
  std::string result =
          "ERROR " + src_ip + ":" + std::to_string(port) + " " + std::to_string(mErrorCode) + " " + "\"" + mErrorMessage +
          "\"\n";

  return result;
}

std::vector<uint8_t> TFTP::OACKPacket::serialize() const {
  std::vector<uint8_t> output;

  output.push_back(0);
  output.push_back(6);

  auto opts = Options::serialize(mOptions);

  output.insert(output.end(), opts.begin(), opts.end());

  return output;
}

std::unique_ptr<TFTP::OACKPacket> TFTP::OACKPacket::deserializeFromData(const std::vector<uint8_t> &data) {
  int idx = 2;

  Options::map_t opts;
  try {
    opts = Options::parse(data, idx);
  } catch (Options::InvalidFormatException &e) {
    throw TFTP::PacketFormatException();
  }

  return std::make_unique<OACKPacket>(opts);
}

std::string TFTP::OACKPacket::formatPacket(std::string src_ip, uint16_t port, uint16_t dst_port) const {
  std::string result = "OACK " + src_ip + ":" + std::to_string(port) + " " + Options::format(mOptions) + "\n";

  return result;
}
