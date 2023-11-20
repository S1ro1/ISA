#include "tftp/Client.h"
#include "utils/ArgParser.h"

int main(int argc, char *argv[]) {
  ClientArgs args = ArgParser::parseClientArgs(argv, argc);
  Options::map_t opts = Options::create(512, 10, 0);

  TFTP::Client client{args, opts};

  client.transmit();

  return 0;
}