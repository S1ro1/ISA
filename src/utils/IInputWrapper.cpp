// Matej Sirovatka, xsirov00

#include "IInputWrapper.h"

void NetAscii::InputWrapper::push(char *os, char c, std::streamsize n) {
  switch (c) {
    case '\r':
      os[mSize++] = '\r';
      if (mSize == n) {
        mLastChar = '\0';
      } else {
        os[mSize++] = '\0';
      }
      break;
    case '\n':
      os[mSize++] = '\r';
      if (mSize == n) {
        mLastChar = '\n';
      } else {
        os[mSize++] = '\n';
      }
      break;
    default:
      os[mSize++] = c;
      break;
  }
}

void NetAscii::InputWrapper::flush(char *os, std::streamsize n) {
  mSize = 0;
  if (mLastChar.has_value()) {
    os[mSize++] = mLastChar.value();
    mLastChar = std::nullopt;
  }
}

NetAscii::InputFile::InputFile(const std::string &filename) {
  mFile.open(filename, std::ios::binary);
}

NetAscii::InputFile::~InputFile() {
  mFile.close();
}


void NetAscii::InputFile::read(char *os, std::streamsize n) {
  flush(os, n);

  while (!mFile.eof() && mSize != n) {
    char c;

    mFile.get(c);
    if (mFile.eof()) {
      break;
    }

    push(os, c, n);
  }
}

void NetAscii::InputStdin::read(char *os, std::streamsize n) {
  flush(os, n);

  while (!std::cin.eof() && mSize != n) {
    char c;

    std::cin.get(c);
    if (std::cin.eof()) {
      break;
    }

    push(os, c, n);
  }
}

Octet::InputFile::InputFile(const std::string &filename) {
  mFile.open(filename, std::ios::binary);
}

Octet::InputFile::~InputFile() {
  mFile.close();
}

void Octet::InputFile::read(char *os, std::streamsize n) {
  mFile.read(os, n);
  mSize = mFile.gcount();
}

void Octet::InputStdin::read(char *os, std::streamsize n) {
  std::cin.read(os, n);
  mSize = std::cin.gcount();
}