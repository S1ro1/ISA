//
// Created by Matej Sirovatka on 14.11.2023.
//

#include "IOutputWrapper.h"

namespace NetAscii {
  OutputFile::OutputFile(const std::string &filename) {
    mFile.open(filename, std::ios::binary);
  }

  OutputFile::~OutputFile() {
    mFile.close();
  }

  void OutputFile::write(const std::vector<uint8_t> &buffer) {

    for (auto c: buffer) {
      if (c == '\r') {
        if (mWasCr) mFile << '\r';
        mWasCr = true;
        continue;
      }

      if (mWasCr) {
        switch (c) {
          case '\n':
            mFile << '\n';
            mWasCr = false;
            break;
          case '\0':
            mFile << '\r';
            mWasCr = false;
            break;
          default:
            mFile << '\r' << c;
            mWasCr = false;
            break;
        }
      } else {
        mFile << c;
      }
    }
  }
}// namespace NetAscii

namespace Octet {
  OutputFile::OutputFile(const std::string &filename) {
    mFile.open(filename, std::ios::binary);
  }

  OutputFile::~OutputFile() {
    mFile.close();
  }

  void OutputFile::write(const std::vector<uint8_t> &buffer) {
    mFile.write(reinterpret_cast<const char *>(buffer.data()), buffer.size());
  }
}// namespace Octet
