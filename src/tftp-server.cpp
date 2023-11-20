#include "tftp/Server.h"
#include "utils/ArgParser.h"

int main(int argc, char *argv[]) {
  ServerArgs args = ArgParser::parseServerArgs(argv, argc);

  TFTP::Server server{args};

  server.listen();

  return 0;
}
