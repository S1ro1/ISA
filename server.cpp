#include <iostream>

#include "utils/ArgParser.h"
#include "utils/TFTPServer.h"

int main(int argc, char *argv[]) {
    std::cout << "Hello from server" << std::endl;
    ServerArgs args = ArgParser::parseServerArgs(argv, argc);

    TFTPServer server{args};

    server.listen();

    return 0;
}
