//
// Created by Matej Sirovatka on 18.11.2023.
//

#ifndef ISA_PROJECT_OPTIONS_H
#define ISA_PROJECT_OPTIONS_H

#include <variant>
#include <map>
#include <string>
#include <vector>
#include <stdexcept>
#include <iostream>


namespace Options {
  using value_t = std::variant<std::string, long>;
  using map_t = std::map<int, std::pair<std::string, value_t>>;

  class InvalidFormatException final : public std::runtime_error {
  public:
    InvalidFormatException() : std::runtime_error("Invalid options format") {}
  };

  class InvalidValueException final : public std::runtime_error {
  public:
    InvalidValueException() : std::runtime_error("Invalid option value") {}
  };

  map_t parse(std::vector<uint8_t> data, int start);

  map_t create(long blksize, long timeout, long tsize);

  [[nodiscard]] std::vector<uint8_t> serialize(const map_t& options);

  [[nodiscard]] std::string format(const map_t& options);

  [[nodiscard]] map_t validate(const map_t& options);

  [[nodiscard]] long get(const std::string& key, const map_t& options);

  long validateInRange(const std::string& value, long min, long max);
}

#endif//ISA_PROJECT_OPTIONS_H
