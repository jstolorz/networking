cmake_minimum_required(VERSION 3.21)
project(networking)

set(CMAKE_CXX_STANDARD 20)

add_subdirectory(simple_example)
add_subdirectory(net_common)
add_subdirectory(net_client)
add_subdirectory(net_server)
add_subdirectory(http_client)


