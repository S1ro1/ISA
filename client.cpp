#include <iostream>

#include "utils/ArgParser.h"

int main(int argc, char* argv[]) {
    std::cout << "Hello from client" << std::endl;
    ClientArgs args = ArgParser::parseClientArgs(argv, argc);

    std::cout << args << std::endl;
    return 0;
}