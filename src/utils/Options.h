// Matej Sirovatka, xsirov00

#ifndef ISA_PROJECT_OPTIONS_H
#define ISA_PROJECT_OPTIONS_H

#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>


namespace Options {
  using value_t = std::variant<std::string, long>;
  using map_t = std::map<int, std::tuple<std::string, value_t, bool>>;

  /**
   * @brief Exception thrown when invalid option format is detected
   */
  class InvalidFormatException final : public std::runtime_error {
  public:
    InvalidFormatException() : std::runtime_error("Invalid mOptions format") {}
  };

  /**
   * @brief Exception thrown when invalid option value is detected
   */
  class InvalidValueException final : public std::runtime_error {
  public:
    InvalidValueException() : std::runtime_error("Invalid option value") {}
  };

  /**
   * @brief parses options from buffer
   * @param data buffer of bytes
   * @param start starting index
   * @return parsed options
   */
  map_t parse(std::vector<uint8_t> data, int start);

  /**
   * @brief creates map of options
   * @param blksize blocksize
   * @param timeout timeout
   * @param tsize tsize
   * @return map of options with specified values
   */
  map_t create(long blksize, long timeout, long tsize);

  /**
   * @brief serializes options to byte vector
   * @param options options to be serialized
   * @return byte vector of serialized options
   */
  [[nodiscard]] std::vector<uint8_t> serialize(const map_t &options);

  /**
   * @brief formats options to string
   * @param options options to be formatted
   * @return formatted string of options
   */
  [[nodiscard]] std::string format(const map_t &options);

  /**
   * @brief validates options
   * @param options options to be validated
   * @return validated options
   */
  [[nodiscard]] map_t validate(const map_t &options);

  /**
   * @brief gets option value
   * @param key key of the option
   * @param options options to look in
   * @return long value of the option
   */
  [[nodiscard]] long get(const std::string &key, const map_t &options);

  /**
   * @brief validates option value
   * @param value value to be validated
   * @param min min range
   * @param max max range
   * @return long value of the option, throws exception if value is invalid
   */
  long validateInRange(const std::string &value, long min, long max);

  /**
   * @brief checks if any of the options is set
   * @param options options to be checked
   * @return true if any is set, false otherwise
   */
  bool isAny(const map_t &options);

  /**
   * @brief checks if option is set
   * @param key key to check
   * @param options options to be checked
   * @return true if value is set, false otherwise
   */
  bool isSet(const std::string &key, const map_t &options);
}// namespace Options

#endif//ISA_PROJECT_OPTIONS_H
