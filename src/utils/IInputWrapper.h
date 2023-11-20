// Matej Sirovatka, xsirov00

#ifndef ISA_TEST_IINPUTWRAPPER_H
#define ISA_TEST_IINPUTWRAPPER_H

#include <fstream>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

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

  /**
   * @return true if input stream is open
   */
  [[nodiscard]] virtual bool is_open() const { return true; }
  /**
   * @return true if end of file was reached
   */
  [[nodiscard]] virtual bool eof() const = 0;
};


/**
 * @brief Wrapper that converts input to netascii
 */
namespace NetAscii {
  class InputWrapper : public IInputWrapper {
  protected:
    std::optional<char> mLastChar = std::nullopt;
    /**
     * pushes character to output stream, converts it to netascii if needed
     * @param os output stream
     * @param c character
     * @param n number of characters to read
     */
    void push(char *os, char c, std::streamsize n);

    /**
     * @brief empties buffer holding overflowing characters
     * @param os output stream
     * @param n number of characters to read
     */
    void flush(char *os, std::streamsize n);
  };

  /**
   * @brief Wrapper for netascii file input
   */
  class InputFile : public InputWrapper {
    std::ifstream mFile;

  public:
    explicit InputFile(const std::string &filename);
    ~InputFile() override;
    void read(char *os, std::streamsize n) override;
    bool is_open() const override { return mFile.is_open(); }
    bool eof() const override { return mFile.eof(); }
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
}// namespace NetAscii


/**
 * @brief Wrapper that converts input to octet
 */
namespace Octet {
  class InputWrapper : public IInputWrapper {};

  /**
   * @brief Wrapper for octet file input
   */
  class InputFile : public InputWrapper {
    std::ifstream mFile;

  public:
    explicit InputFile(const std::string &filename);
    ~InputFile() override;
    void read(char *os, std::streamsize n) override;
    bool is_open() const override { return mFile.is_open(); }
    bool eof() const override { return mFile.eof(); }
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
}// namespace Octet

#endif//ISA_TEST_IINPUTWRAPPER_H
