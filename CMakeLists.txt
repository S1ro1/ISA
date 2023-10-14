cmake_minimum_required(VERSION 3.25)
project(isa_project)

set(CMAKE_CXX_STANDARD 23)

add_executable(isa_server server.cpp utils/ArgParser.cpp utils/ArgParser.h utils/utils.h utils/TFTPPacket.cpp utils/TFTPPacket.h utils/TFTPServer.cpp utils/TFTPServer.h)
add_executable(isa_client client.cpp utils/ArgParser.cpp utils/ArgParser.h utils/TFTPPacket.cpp utils/TFTPPacket.h utils/TFTPClient.cpp utils/TFTPClient.h utils/utils.h)

target_link_libraries(isa_client -Wl,-ld_classic)
target_link_libraries(isa_server -Wl,-ld_classic)
