cmake_minimum_required(VERSION 3.18)

project(client VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(SOURCES ./client_udp.cpp)

add_subdirectory(../3rdparty/kcp-1.7 kcp_binary_dir)
add_executable(client ${SOURCES})

target_link_libraries(client kcp)