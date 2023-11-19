//
// Created by Matej Sirovatka on 14.11.2023.
//

#ifndef ISA_TEST_IOUTPUTWRAPPER_H
#define ISA_TEST_IOUTPUTWRAPPER_H

#include <vector>
#include <iostream>
#include <fstream>

class IOutputWrapper {
public:
  virtual ~IOutputWrapper() = default;
  virtual void write(const std::vector<uint8_t>& buffer) = 0;
  virtual bool is_open() const = 0;
  virtual bool good() const = 0;
};

namespace NetAscii {
  class OutputFile : public IOutputWrapper {
    char mWasCr = false;
    std::ofstream mFile;
  public:
    bool is_open() const override { return mFile.is_open(); }
    bool good() const override { return mFile.good(); }
    explicit OutputFile(const std::string& filename);
    ~OutputFile() override;
    void write(const std::vector<uint8_t>& buffer) override;
  };
}

namespace Octet {
  class OutputFile : public IOutputWrapper {
    std::ofstream mFile;
  public:
    bool is_open() const override { return mFile.is_open(); }
    bool good() const override { return mFile.good(); }
    explicit OutputFile(const std::string& filename);
    ~OutputFile() override;
    void write(const std::vector<uint8_t>& buffer) override;
  };
}


#endif//ISA_TEST_IOUTPUTWRAPPER_H
