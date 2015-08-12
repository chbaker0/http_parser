cmake_minimum_required(VERSION 3.1)
project(http-parser)

set(BIN_INSTALL_DIR bin CACHE PATH "Installation directory for binaries")
set(LIB_INSTALL_DIR lib CACHE PATH "Installation directory for libraries")
set(INCLUDE_INSTALL_DIR include CACHE PATH "Installation directory for includes")
set(CONFIG_INSTALL_DIR lib/cmake/http-parser CACHE PATH "Installation directory for CMake package config files")

foreach(p LIB INCLUDE CONFIG)
    set(var ${p}_INSTALL_DIR)
	if(NOT IS_ABSOLUTE "${${var}}")
        set(${var} "${CMAKE_INSTALL_PREFIX}/${${var}}")
    endif()
endforeach()

include(CMakePackageConfigHelpers)
configure_package_config_file(http-parser-config.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/http-parser-config.cmake
    INSTALL_DESTINATION ${CONFIG_INSTALL_DIR}
	PATH_VARS INCLUDE_INSTALL_DIR LIB_INSTALL_DIR
)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/http-parser-config.cmake DESTINATION ${CONFIG_INSTALL_DIR})

include_directories(AFTER include)

add_subdirectory(src)
add_subdirectory(include)
add_subdirectory(tests)
