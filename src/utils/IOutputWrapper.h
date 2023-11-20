// Matej Sirovatka, xsirov00

#ifndef ISA_TEST_IOUTPUTWRAPPER_H
#define ISA_TEST_IOUTPUTWRAPPER_H

#include <fstream>
#include <iostream>
#include <vector>

/**
 * @brief Base class for output wrappers
 */
class IOutputWrapper {
public:
  virtual ~IOutputWrapper() = default;
  /**
   * @brief writes buffer to output stream
   * @param buffer
   */
  virtual void write(const std::vector<uint8_t> &buffer) = 0;
  /**
   * @return true if output stream is open
   */
  virtual bool is_open() const = 0;
  /**
   * @return true if output stream is in good state
   */
  virtual bool good() const = 0;
};

namespace NetAscii {
  class OutputFile : public IOutputWrapper {
    char mWasCr = false;
    std::ofstream mFile;

  public:
    bool is_open() const override { return mFile.is_open(); }
    bool good() const override { return mFile.good(); }
    explicit OutputFile(const std::string &filename);
    ~OutputFile() override;
    /**
     * @brief writes buffer, converted from netascii to unix format, to output stream
     * @param buffer buffer to be written
     */
    void write(const std::vector<uint8_t> &buffer) override;
  };
}// namespace NetAscii

namespace Octet {
  class OutputFile : public IOutputWrapper {
    std::ofstream mFile;

  public:
    bool is_open() const override { return mFile.is_open(); }
    bool good() const override { return mFile.good(); }
    explicit OutputFile(const std::string &filename);
    ~OutputFile() override;
    void write(const std::vector<uint8_t> &buffer) override;
  };
}// namespace Octet


#endif//ISA_TEST_IOUTPUTWRAPPER_H
