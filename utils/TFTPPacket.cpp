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
      return ACKPacket::deserializeFromData(data);
    case 5:
      return ErrorPacket::deserializeFromData(data);
    case 6:
      return OACKPacket::deserializeFromData(data);
    default:
      throw PacketOpCodeError(opcode);
  }
}

std::vector<uint8_t> RRQPacket::serialize() const {
  std::vector<uint8_t> output;

  output.push_back(0);
  output.push_back(1);
  for (char c: filename) {
    output.push_back(static_cast<uint8_t>(c));
  }

  output.push_back(0);

  for (char c: mode) {
    output.push_back(static_cast<uint8_t>(c));
  }

  output.push_back(0);

  auto opts = options.serialize();

  output.insert(output.end(), opts.begin(), opts.end());

  return output;
}

std::unique_ptr<RRQPacket> RRQPacket::deserializeFromData(const std::vector<uint8_t> &data) {
  int idx = 2;

  std::string fname;
  while (data[idx] && idx < data.size()) {
    fname += static_cast<char>(data[idx++]);
  }

  idx++;

  std::string mode;

  while (data[idx] && idx < data.size()) {
    mode += static_cast<char>(data[idx++]);
  }

  idx++;

  OptionsMap opts = OptionsMap(data, idx);


  return std::make_unique<RRQPacket>(fname, mode, opts);
}

std::string RRQPacket::formatPacket(std::string src_ip, uint16_t port, uint16_t dst_port) const {
  std::string result = "RRQ " + src_ip + ":" + std::to_string(port) + " " + "\"" + filename + "\" " + mode;

  result += " " + options.format();

  result += "\n";

  return result;
}

std::vector<uint8_t> WRQPacket::serialize() const {
  std::vector<uint8_t> output;

  output.push_back(0);
  output.push_back(2);
  for (char c: filename) {
    output.push_back(static_cast<uint8_t>(c));
  }

  output.push_back(0);

  for (char c: mode) {
    output.push_back(static_cast<uint8_t>(c));
  }

  output.push_back(0);

  auto opts = options.serialize();

  output.insert(output.end(), opts.begin(), opts.end());

  return output;
}

std::unique_ptr<WRQPacket> WRQPacket::deserializeFromData(const std::vector<uint8_t> &data) {
  int idx = 2;

  std::string fname;

  while (data[idx] && idx < data.size()) {
    fname += static_cast<char>(data[idx++]);
  }

  idx++;

  std::string mode;

  while (data[idx] && idx < data.size()) {
    mode += static_cast<char>(data[idx++]);
  }

  idx++;

  OptionsMap opts = OptionsMap(data, idx);
  return std::make_unique<WRQPacket>(fname, mode, opts);
}

std::string WRQPacket::formatPacket(std::string src_ip, uint16_t port, uint16_t dst_port) const {
  std::string result = "WRQ " + src_ip + ":" + std::to_string(port) + " " + "\"" + filename + "\" " + mode;

  result += " " + options.format();

  result += "\n";

  return result;
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
  if (data.size() < 4) {
    return nullptr;
  }

  uint16_t blockNum = (data[2] << 8) | data[3];

  std::vector<uint8_t> outData(data.begin() + 4, data.end());

  return std::make_unique<DataPacket>(blockNum, outData);
}

std::string DataPacket::formatPacket(std::string src_ip, uint16_t port, uint16_t dst_port) const {
  std::string result = "DATA " + src_ip + ":" + std::to_string(port) + ":" + std::to_string(dst_port) + " " +
                       std::to_string(blockNumber) + "\n";

  return result;
}

std::vector<uint8_t> ACKPacket::serialize() const {
  std::vector<uint8_t> output;

  output.push_back(0);
  output.push_back(4);

  output.push_back((blockNumber >> 8) & 0xFF);
  output.push_back(blockNumber & 0xFF);

  return output;
}

std::unique_ptr<ACKPacket> ACKPacket::deserializeFromData(const std::vector<uint8_t> &data) {
  if (data.size() != 4) {
    return nullptr;
  }

  uint16_t blockNum = (data[2] << 8) | data[3];

  return std::make_unique<ACKPacket>(blockNum);
}

std::string ACKPacket::formatPacket(std::string src_ip, uint16_t port, uint16_t dst_port) const {
  std::string result = "ACK " + src_ip + ":" + std::to_string(port) + " " + std::to_string(blockNumber) + "\n";

  return result;
}

std::vector<uint8_t> ErrorPacket::serialize() const {
  std::vector<uint8_t> output;

  output.push_back(0);
  output.push_back(5);

  output.push_back((errorCode >> 8) & 0xFF);
  output.push_back(errorCode & 0xFF);

  for (char c: errorMsg) {
    output.push_back(static_cast<uint8_t>(c));
  }

  output.push_back(0);

  return output;
}

std::unique_ptr<ErrorPacket> ErrorPacket::deserializeFromData(const std::vector<uint8_t> &data) {
  if (data.size() < 4) {
    return nullptr;
  }

  uint16_t errorCode = (data[2] << 8) | data[3];

  int idx = 4;
  std::string errorMessage;
  while (data[idx] && idx < data.size()) {
    errorMessage += static_cast<char>(data[idx++]);
  }

  return std::make_unique<ErrorPacket>(errorCode, errorMessage);
}

std::string ErrorPacket::formatPacket(std::string src_ip, uint16_t port, uint16_t dst_port) const {
  std::string result =
          "ERROR " + src_ip + ":" + std::to_string(port) + " " + std::to_string(errorCode) + " " + "\"" + errorMsg +
          "\"\n";

  return result;
}

std::vector<uint8_t> OACKPacket::serialize() const {
  std::vector<uint8_t> output;

  output.push_back(0);
  output.push_back(6);

  auto opts = options.serialize();

  output.insert(output.end(), opts.begin(), opts.end());

  return output;
}

std::unique_ptr<OACKPacket> OACKPacket::deserializeFromData(const std::vector<uint8_t> &data) {
  int idx = 2;

  OptionsMap opts = OptionsMap(data, idx);

  return std::make_unique<OACKPacket>(opts);
}

std::string OACKPacket::formatPacket(std::string src_ip, uint16_t port, uint16_t dst_port) const {
  std::string result = "OACK " + src_ip + ":" + std::to_string(port) + " " + options.format() + "\n";

  return result;
}

OptionsMap::OptionsMap(const std::vector<uint8_t> &data, size_t start) {
  const std::set<std::string> validOptions = {
          "blksize",
          "timeout",
          "tsize"};

  mTimeout = std::pair{5, false};
  mBlksize = std::pair{512, false};
  mTsize = std::pair{0, false};


  std::string key, value;
  bool isKey = true;
  int intVal;

  for (size_t i = start; i < data.size(); i++) {
    if (data[i] == 0) {
      if (isKey) {
        key = value;
        value = "";
        isKey = false;
      } else {
        if (validOptions.find(key) == validOptions.end()) {
          throw PacketOptionError(key);
        }
        try {
          intVal = std::stoi(value);
        } catch (std::invalid_argument &e) {
          throw PacketOptionError(key);
        }

        if (key == "blksize") {
          mBlksize = std::pair{intVal, true};
        } else if (key == "timeout") {
          mTimeout = std::pair{intVal, true};
        } else if (key == "tsize") {
          mTsize = std::pair{intVal, true};
        }
        value = "";
        isKey = true;
      }
    } else {
      value += static_cast<char>(data[i]);
    }
  }
}

std::vector<uint8_t> OptionsMap::serialize() const {
  std::vector<uint8_t> output;
  std::array<std::string, 3> keys {"blksize", "timeout", "tsize" };

  std::string currValue;

  for (auto &key : keys) {

    if (key == "blksize") {
      if (mBlksize.second) {
        currValue = std::to_string(mBlksize.first);
      } else {
        continue;
      }
    } else if (key == "timeout") {
      if (mTimeout.second) {
        currValue = std::to_string(mTimeout.first);
      } else {
        continue;
      }
    } else if (key == "tsize") {
      if (mTsize.second) {
        currValue = std::to_string(mTsize.first);
      } else {
        continue;
      }
    }

    for (auto c: key) {
      output.push_back(static_cast<uint8_t>(c));
    }
    output.push_back(0);

    for (auto c: currValue) {
      output.push_back(static_cast<uint8_t>(c));
    }

    output.push_back(0);
  }

  std::string outputString(output.begin(), output.end());
  return output;
}
