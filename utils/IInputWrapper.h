//
// Created by Matej Sirovatka on 14.11.2023.
//

#ifndef ISA_TEST_IINPUTWRAPPER_H
#define ISA_TEST_IINPUTWRAPPER_H

#include <string>
#include <sstream>
#include <fstream>
#include <iostream>
#include <vector>
#include <optional>

/**
 * @brief Base class for input wrappers
 */
class IInputWrapper {

protected:
  std::streamsize mSize = 0;

public:
    /**
     * @return number of characters read by last read() call
     */
    [[nodiscard]] virtual std::streamsize gcount() const { return mSize; }
    virtual ~IInputWrapper() = default;
    /**
     * @brief reads n characters from input stream and stores them in os
     * @param os array to store characters in
     * @param n number of characters to read
     */
    virtual void read(char *os, std::streamsize n) = 0;

    [[nodiscard]] virtual bool is_open() const { return true; }
    [[nodiscard]] virtual bool eof() const = 0;
};


/**
 * @brief Wrapper that converts input to netascii
 */
namespace NetAscii {
  class InputWrapper : public IInputWrapper {
  protected:
    std::optional<char> mLastChar = std::nullopt;
    void push(char *os, char c, std::streamsize n);
    void flush(char *os, std::streamsize n);
  };

  /**
   * @brief Wrapper for netascii file input
   */
  class InputFile : public InputWrapper {
    std::ifstream file;
  public:
    explicit InputFile(const std::string& filename);
    ~InputFile() override;
    void read(char *os, std::streamsize n) override;
    bool is_open() const override { return file.is_open(); }
    bool eof() const override { return file.eof(); }
  };

  /**
   * @brief Wrapper for netascii stdin input
   */
  class InputStdin : public InputWrapper {
  public:
    ~InputStdin() override = default;
    void read(char *os, std::streamsize n) override;
    bool eof() const override { return std::cin.eof(); }
  };
}


/**
 * @brief Wrapper that converts input to octet
 */
namespace Octet {
  class InputWrapper : public IInputWrapper {};

  /**
   * @brief Wrapper for octet file input
   */
  class InputFile : public InputWrapper {
    std::ifstream file;
  public:
    explicit InputFile(const std::string& filename);
    ~InputFile() override;
    void read(char *os, std::streamsize n) override;
    bool is_open() const override { return file.is_open(); }
    bool eof() const override { return file.eof(); }
  };

  /**
   * @brief Wrapper for octet stdin input
   */
  class InputStdin : public InputWrapper {
  public:
    ~InputStdin() override = default;
    void read(char *os, std::streamsize n) override;
    bool eof() const override { return std::cin.eof(); }
  };
}

#endif//ISA_TEST_IINPUTWRAPPER_H
