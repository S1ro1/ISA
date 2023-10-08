#include <iostream>

#include "utils/ArgParser.h"
#include "utils/TFTPClient.h"

int main(int argc, char *argv[]) {
    std::cout << "Hello from client" << std::endl;
    ClientArgs args = ArgParser::parseClientArgs(argv, argc);

    TFTPClient client{args};

    client.transmit();

    return 0;
}