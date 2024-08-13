cmake_minimum_required(VERSION 3.13)
project(test-task)

set(CMAKE_CXX_STANDARD 20)

FILE(GLOB server_src server*.cpp)
add_executable(server ${server_src})
target_include_directories(server PRIVATE .)
target_link_libraries(server pthread boost_thread)

FILE(GLOB client_src client*.cpp)
add_executable(client ${client_src})
target_include_directories(client PRIVATE .)
target_link_libraries(client pthread boost_thread)
