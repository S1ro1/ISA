#include <iostream>

#include "utils/ArgParser.h"
#include "utils/TFTPClient.h"

int main(int argc, char *argv[]) {
    ClientArgs args = ArgParser::parseClientArgs(argv, argc);

    TFTPClient client{args};

    client.transmit();

    return 0;
}