cmake_minimum_required (VERSION 3.5.0)

project (rest VERSION 1.0.0 LANGUAGES CXX)

include (GNUInstallDirs)

# Dependencies
find_package (Boost 1.66.0 REQUIRED system filesystem)
find_package (Threads REQUIRED)

# General project requirements
set (CMAKE_CXX_STANDARD 11)
set (CMAKE_CXX_STANDARD_REQUIRED ON)

# General project flags
add_compile_options (-Wall -Wextra -Wpedantic -pedantic -pedantic-errors)

# Configuration
option (EXAMPLES "Enable build of examples" OFF)

# Library
add_library (rest INTERFACE)
target_include_directories (rest INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include/rest>)

install (DIRECTORY rest
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")

install (FILES rest.hpp
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")

# Examples
if (EXAMPLES)
    message (STATUS "Adding examples...")
    add_subdirectory (examples "${CMAKE_BINARY_DIR}/examples")
endif ()

