//
// Created by Matej Sirovatka on 08.10.2023.
//

#ifndef ISA_PROJECT_ARGPARSER_H
#define ISA_PROJECT_ARGPARSER_H

#include <string>
#include <optional>
#include <iostream>

#include <unistd.h>

struct ClientArgs {
    std::string address;
    uint32_t port;

    std::optional<std::string> src_file_path;
    std::string dst_file_path;

public:
    friend std::ostream& operator <<(std::ostream& os, const ClientArgs& obj);
};

struct ServerArgs {
    uint32_t port;
    std::string root_dir;

public:
    friend std::ostream& operator <<(std::ostream& os, const ServerArgs& obj);
};

class ArgParser {
public:
    static ClientArgs parseClientArgs(char* argv[], int argc);
    static ServerArgs parseServerArgs(char* argv[], int argc);
};


#endif //ISA_PROJECT_ARGPARSER_H
