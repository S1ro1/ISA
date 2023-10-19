//
// Created by Matej Sirovatka on 08.10.2023.
//

#include "ArgParser.h"


ClientArgs ArgParser::parseClientArgs(char *argv[], int argc) {
    int opt;

    ClientArgs args{
            .address = std::string(),
            .port = 69,
            .src_file_path = std::nullopt,
            .dst_file_path = std::string(),
    };

    while ((opt = getopt(argc, argv, "h:p:f:t:")) != -1) {
        switch (opt) {
            case 'h':
                args.address = optarg;
                break;
            case 'p':
                args.port = std::strtol(optarg, nullptr, 10);
                break;
            case 'f':
                args.src_file_path = optarg;
                break;
            case 't':
                args.dst_file_path = optarg;
                break;
            default:
                std::cerr << "Wrong arguments specified" << std::endl;
                exit(2);
        }
    }

    if (args.address.empty() or args.dst_file_path.empty()) {
        std::cerr << "Wrong arguments specified" << std::endl;
        exit(2);
    }

    return args;
}


std::ostream &operator<<(std::ostream &os, const ClientArgs &obj) {
    os << "Address: " << obj.address << std::endl;
    os << "Port: " << obj.port << std::endl;

    std::string path;
    if (obj.src_file_path.has_value()) {
        path = obj.src_file_path.value();
    } else {
        path = "stdin";
    }

    os << "Src file path: " << path << std::endl;
    os << "Dst file path: " << obj.dst_file_path << std::endl;

    return os;
}

ServerArgs ArgParser::parseServerArgs(char *argv[], int argc) {
    int opt;

    ServerArgs args{
            .port = 69,
            .root_dir = std::string()
    };

    while ((opt = getopt(argc, argv, "p:")) != -1) {
        switch (opt) {
            case 'p':
                args.port = std::strtol(optarg, nullptr, 10);
                break;
            default:
                std::cerr << "Wrong arguments specified" << std::endl;
                exit(2);
        }
    }

    for (int i = optind; i < argc; ++i) {
        if (args.root_dir.empty()) {
            if (strlen(argv[i]) != 0) {
                args.root_dir = argv[i];
            }
        } else {
            std::cerr << "Wrong arguments specified" << std::endl;
            exit(2);
        }
    }

    if (args.root_dir.empty()) {
        std::cerr << "Wrong arguments specified" << std::endl;
        exit(2);
    }

    return args;
}

std::ostream &operator<<(std::ostream &os, const ServerArgs &obj) {
    os << "Port: " << obj.port << std::endl;
    os << "Root dir: " << obj.root_dir << std::endl;

    return os;
}
