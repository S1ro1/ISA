cmake_minimum_required(VERSION 3.25)
project(isa_project)

set(CMAKE_CXX_STANDARD 23)

add_executable(isa_server server.cpp utils/ArgParser.cpp utils/ArgParser.h)
add_executable(isa_client client.cpp utils/ArgParser.cpp utils/ArgParser.h utils/TFTPPacket.cpp utils/TFTPPacket.h utils/TFTPClient.cpp utils/TFTPClient.h)
