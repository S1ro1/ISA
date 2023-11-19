
#include "utils/ArgParser.h"
#include "utils/TFTPClient.h"

int main(int argc, char *argv[]) {
  ClientArgs args = ArgParser::parseClientArgs(argv, argc);
  Options::map_t opts = Options::create(512, 10, 0);

  TFTPClient client{args, opts};

  client.transmit();

  return 0;
}