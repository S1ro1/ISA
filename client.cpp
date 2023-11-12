#include <iostream>

#include "utils/ArgParser.h"
#include "utils/TFTPClient.h"

int main(int argc, char *argv[]) {
  ClientArgs args = ArgParser::parseClientArgs(argv, argc);

  OptionsMap opts {512, 5, 0};

  TFTPClient client{args, opts};

  client.transmit();

  return 0;
}