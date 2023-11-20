// Matej Sirovatka, xsirov00

#ifndef ISA_PROJECT_ARGPARSER_H
#define ISA_PROJECT_ARGPARSER_H

#include <cstring>
#include <iostream>
#include <optional>
#include <string>

#include <unistd.h>

/**
 * @brief Structure holding arguments passed to the client program
 */
struct ClientArgs {
  std::string mAddress;
  uint32_t mPort;

  std::optional<std::string> mSrcFilePath;
  std::string mDestFilePath;

public:
  friend std::ostream &operator<<(std::ostream &os, const ClientArgs &obj);
};

/**
 * @brief Structure holding arguments passed to the server program
 */
struct ServerArgs {
  uint32_t mPort;
  std::string mRootDir;

public:
  friend std::ostream &operator<<(std::ostream &os, const ServerArgs &obj);
};

class ArgParser {
public:
  /**
   * @brief Parses arguments passed to the client program
   * @param argv number of arguments
   * @param argc array of arguments
   * @return parsed arguments
   */
  static ClientArgs parseClientArgs(char *argv[], int argc);

  /**
   * @brief Parses arguments passed to the server program
   * @param argv number of arguments
   * @param argc array of arguments
   * @return parsed arguments
   */
  static ServerArgs parseServerArgs(char *argv[], int argc);
};


#endif//ISA_PROJECT_ARGPARSER_H
