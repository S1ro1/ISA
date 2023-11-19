//
// Created by Matej Sirovatka on 18.11.2023.
//

#include "Options.h"

namespace Options {
  map_t parse(std::vector<uint8_t> data, int start) {
    map_t options = {};

    bool inKey = true;
    std::string key;
    std::string value;
    int numOptions = 0;

    for (int idx = start; idx < data.size(); idx++) {
      if (data[idx] == 0) {
        if (inKey) {
          inKey = false;
        } else {
          options[numOptions++] = std::tuple(key, value, false);
          key.clear();
          value.clear();
          inKey = true;
          continue;
        }
      } else {
        if (inKey) {
          key += static_cast<char>(data[idx]);
        } else {
          value += static_cast<char>(data[idx]);
        }
      }
    }

    if (!key.empty()) throw InvalidFormatException();
    if (!value.empty()) throw InvalidFormatException();

    if (!inKey) throw InvalidFormatException();

    return options;
  }

  map_t create(long blksize, long timeout, long tsize) {
    map_t options;

    options[0] = std::tuple("blksize", blksize, false);
    options[1] = std::tuple("timeout", timeout, false);
    options[2] = std::tuple("tsize", tsize, false);

    return options;
  }

  std::vector<uint8_t> serialize(const map_t& options) {
    std::vector<uint8_t> output;

    for (const auto&[order, item]: options) {
      const auto&[key, value, set] = item;

      output.insert(output.end(), key.begin(), key.end());
      output.push_back(0);

      if (std::holds_alternative<std::string>(value)) {
        const auto& str = std::get<std::string>(value);
        output.insert(output.end(), str.begin(), str.end());
      } else {
        const auto& num = std::get<long>(value);
        const auto& str = std::to_string(num);
        output.insert(output.end(), str.begin(), str.end());
      }

      output.push_back(0);
    }
    return output;
  }

  std::string format(const map_t &options) {
    std::string result;

    for (auto &[order, item]: options) {
      auto &[key, value, set] = item;

      if (std::holds_alternative<std::string>(value)) {
        result += key + "=" + std::get<std::string>(value);
      } else {
        result += key + "=" + std::to_string(std::get<long>(value));
      }

      result += " ";
    }

    result.pop_back();
    return result;
  }

  map_t validate(const map_t &options) {
    map_t validated{};

    validated[0] = std::tuple("blksize", 512, false);
    validated[1] = std::tuple("timeout", 5, false);
    validated[2] = std::tuple("tsize", 0, false);

    for (const auto &[order, item]: options) {
      const auto &[key, value, set] = item;
      long result;

      // we expect this to be called only as it holds only string values
      auto str = std::get<std::string>(value);

      if (key == "blksize") {
        try {
          result = validateInRange(str, 8, 65464);
        } catch (InvalidValueException &e) {
          result = 512;
        }
        validated[0] = std::tuple("blksize", result, true);
      } else if (key == "timeout") {
        try {
          result = validateInRange(str, 1, 255);
        } catch (InvalidValueException &e) {
          result = 5;
        }
        validated[1] = std::tuple("timeout", result, true);
      } else if (key == "tsize") {
        try {
          result = validateInRange(str, 0, 4294967295);
        } catch (InvalidValueException &e) {
          result = 0;
        }
        validated[2] = std::tuple("tsize", result, true);
      }
    }
    return validated;
  }

  long get(const std::string& key, const map_t& options) {
    for (const auto &[order, item]: options) {
      const auto &[key_, value, set] = item;
      if (key == key_) {
        return std::get<long>(value);
      }
    }

    // unreachable
    return 0;
  }

  long validateInRange(const std::string& value, long min, long max) {
    long result;
    try {
      result = std::stol(value);
    } catch (std::exception& e) {
      throw InvalidValueException();
    }

    if (result < min or result > max) {
      throw InvalidValueException();
    }

    return result;
  }

  bool isAny(const map_t& options) {
    for (auto &[order, item]: options) {
      auto &[key, value, set] = item;
      if (set) return true;
    }
    return false;
  }

  bool isSet(const std::string& key, const map_t& options) {
    for (auto &[order, item]: options) {
      auto &[key_, value, set] = item;
      if (key == key_ && set) return true;
    }
    return false;
  }
}
