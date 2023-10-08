#include <iostream>

#include "utils/ArgParser.h"

int main(int argc, char* argv[]) {
    std::cout << "Hello from server" << std::endl;
    ServerArgs args = ArgParser::parseServerArgs(argv, argc);

    std::cout << args << std::endl;
    return 0;
}
