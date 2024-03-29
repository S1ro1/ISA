// Matej Sirovatka, xsirov00

#include "ArgParser.h"

void printServerHelp() {
  std::cout << "Usage: tftp-server [-p PORT] ROOT_DIR" << std::endl;
}

void printClientHelp() {
  std::cout << "Usage download: tftp-client -h HOST -t DESTINATION_PATH [-p PORT] [-f SOURCE_PATH]" << std::endl;
  std::cout << "Usage upload (reads from stdin): tftp-client -h HOST -t DESTINATION_PATH [-p PORT]" << std::endl;
}

ClientArgs ArgParser::parseClientArgs(char *argv[], int argc) {
  int opt;

  ClientArgs args{
          .mAddress = std::string(),
          .mPort = 69,
          .mSrcFilePath = std::nullopt,
          .mDestFilePath = std::string(),
  };

  while ((opt = getopt(argc, argv, "h:p:f:t:")) != -1) {
    switch (opt) {
      case 'h':
        args.mAddress = optarg;
        break;
      case 'p':
        args.mPort = std::strtol(optarg, nullptr, 10);
        break;
      case 'f':
        args.mSrcFilePath = optarg;
        break;
      case 't':
        args.mDestFilePath = optarg;
        break;
      default:
        printClientHelp();
        exit(2);
    }
  }

  if (args.mAddress.empty() or args.mDestFilePath.empty()) {
    printClientHelp();
    exit(2);
  }

  return args;
}


std::ostream &operator<<(std::ostream &os, const ClientArgs &obj) {
  os << "Address: " << obj.mAddress << std::endl;
  os << "Port: " << obj.mPort << std::endl;

  std::string path;
  if (obj.mSrcFilePath.has_value()) {
    path = obj.mSrcFilePath.value();
  } else {
    path = "stdin";
  }

  os << "Src file path: " << path << std::endl;
  os << "Dst file path: " << obj.mDestFilePath << std::endl;

  return os;
}

ServerArgs ArgParser::parseServerArgs(char *argv[], int argc) {
  int opt;

  ServerArgs args{
          .mPort = 69,
          .mRootDir = std::string()};

  while ((opt = getopt(argc, argv, "p:")) != -1) {
    switch (opt) {
      case 'p':
        args.mPort = std::strtol(optarg, nullptr, 10);
        break;
      default:
        printServerHelp();
        exit(2);
    }
  }

  for (int i = optind; i < argc; ++i) {
    if (args.mRootDir.empty()) {
      if (strlen(argv[i]) != 0) {
        args.mRootDir = argv[i];
      }
    } else {
      printServerHelp();
      exit(2);
    }
  }

  if (args.mRootDir.empty()) {
    printServerHelp();
    exit(2);
  }

  return args;
}

std::ostream &operator<<(std::ostream &os, const ServerArgs &obj) {
  os << "Port: " << obj.mPort << std::endl;
  os << "Root dir: " << obj.mRootDir << std::endl;

  return os;
}
