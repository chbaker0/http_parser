cmake_minimum_required(VERSION 3.1)
project(http_parser)

include_directories(AFTER .)

add_subdirectory(http)
add_subdirectory(tests)
