
#include "utils/ArgParser.h"
#include "utils/TFTPClient.h"

int main(int argc, char *argv[]) {
  ClientArgs args = ArgParser::parseClientArgs(argv, argc);
  Options::map_t opts = Options::create(512, 10, 0);


  std::vector<uint8_t> options = {
    98, 108, 107, 115, 105, 122, 101, 0,    // "blksize"
    53, 49, 50, 0,                          // "512"
    116, 105, 109, 101, 111, 117, 116, 0,   // "timeout"
    53, 0,                                  // "5"
    116, 115, 105, 122, 101, 0,             // "tsize"
    49, 48, 0,                              // "10"
    107, 101, 121, 55, 0,                   // "key7"
    118, 97, 108, 117, 101, 51, 0,          // "value3"
    100, 117, 109, 109, 121, 107, 101, 121, 0, // "dummykey"
    // 100, 117, 109, 109, 121, 118, 97, 108, 117, 101, 0 // "dummyvalue"
  };

//  Options::map_t opts2 = Options::parse(options, 0);

  TFTPClient client{args, opts};

  client.transmit();

  return 0;
}