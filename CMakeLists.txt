cmake_minimum_required(VERSION 3.25)
project(isa_project)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")

add_executable(isa_server src/tftp-server.cpp src/utils/ArgParser.cpp src/utils/ArgParser.h src/utils/utils.h src/tftp/Packet.cpp src/tftp/Packet.h src/tftp/Server.cpp src/tftp/Server.h src/tftp/Connection.cpp src/tftp/Connection.h src/utils/Options.cpp src/utils/Options.h src/tftp/common.h src/utils/IInputWrapper.cpp src/utils/IInputWrapper.h src/utils/IOutputWrapper.cpp src/utils/IOutputWrapper.h)
add_executable(isa_client src/tftp-client.cpp src/utils/ArgParser.cpp src/utils/ArgParser.h src/tftp/Packet.cpp src/tftp/Packet.h src/tftp/Client.cpp src/tftp/Client.h src/utils/utils.h src/utils/Options.cpp src/utils/Options.h src/tftp/common.h src/utils/IInputWrapper.cpp src/utils/IInputWrapper.h src/utils/IOutputWrapper.cpp src/utils/IOutputWrapper.h)

if (DEBUG_LOG)
    target_compile_definitions(isa_server PUBLIC DEBUG_LOG)
    target_compile_definitions(isa_client PUBLIC DEBUG_LOG)
endif ()

target_link_libraries(isa_client -Wl,-ld_classic)
target_link_libraries(isa_server -Wl,-ld_classic)

#target_link_libraries(isa_server pthread)
#target_link_libraries(isa_client pthread)
