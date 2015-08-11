cmake_minimum_required(VERSION 3.1)

macro(add_test target)
add_executable(${target} ${ARGN})
set_target_properties(${target} PROPERTIES CXX_STANDARD 11 CXX_STANDARD_REQUIRED ON)
target_link_libraries(${target} http_parser)
endmacro(add_test)

add_test(parse_file parse_file.cpp)
add_test(parse_stdin parse_stdin.cpp)
