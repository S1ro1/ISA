//
// Created by Matej Sirovatka on 09.10.2023.
//

#ifndef ISA_PROJECT_TFTPCLIENT_H
#define ISA_PROJECT_TFTPCLIENT_H

#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string>
#include <fstream>

#include "TFTPPacket.h"
#include "ArgParser.h"

enum class TFTPState {
    INIT,
    SENT_RRQ,
    SENT_WRQ,
    FINAL_ACK,
    ERROR
};

enum class Mode {
    DOWNLOAD,
    UPLOAD,
};


class TFTPClient {
    int socket_fd;
    sockaddr_in server_address;
    TFTPState state;
    std::string transmissionMode = "octet";

    Mode mode;
    std::string src_file_path;
    std::string dst_file_path;

    OptionsMap opts;

    void sendPacket(const TFTPPacket &packet);

    std::unique_ptr<TFTPPacket> receivePacket();

public:
    explicit TFTPClient(const ClientArgs &args);

    ~TFTPClient() {
        close(socket_fd);
    }

    void transmit();

    void requestRead();

    void requestWrite();

};


#endif //ISA_PROJECT_TFTPCLIENT_H
