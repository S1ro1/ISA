//
// Created by Matej Sirovatka on 08.10.2023.
//

#ifndef ISA_PROJECT_ARGPARSER_H
#define ISA_PROJECT_ARGPARSER_H

#include <cstring>
#include <iostream>
#include <optional>
#include <string>

#include <unistd.h>

struct ClientArgs {
  std::string mAddress;
  uint32_t mPort;

  std::optional<std::string> mSrcFilePath;
  std::string mDestFilePath;

public:
  friend std::ostream &operator<<(std::ostream &os, const ClientArgs &obj);
};

struct ServerArgs {
  uint32_t mPort;
  std::string mRootDir;

public:
  friend std::ostream &operator<<(std::ostream &os, const ServerArgs &obj);
};

class ArgParser {
public:
  static ClientArgs parseClientArgs(char *argv[], int argc);

  static ServerArgs parseServerArgs(char *argv[], int argc);
};


#endif//ISA_PROJECT_ARGPARSER_H
