cmake_minimum_required(VERSION 3.0 FATAL_ERROR)
project(fcfs VERSION 0.1 LANGUAGES C)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DFUSE_USE_VERSION=31 -DDBG -D__FILENAME__='\"$(subst ${CMAKE_SOURCE_DIR}/,,$(abspath $<))\"' -D_FILE_OFFSET_BITS=64 -g -Wall -lm")
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/CMake" ${CMAKE_MODULE_PATH})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

#find_package(FUSE REQUIRED)

#include_directories(${FUSE_INCLUDE_DIR})
include_directories("/home/snippy/diploma/include/")
link_directories("/home/snippy/diploma/lib/lib/")

add_executable(fcfs fcfs.c utils.c fcfs_opts.c fcfs_mount.c fcfs_cache.c)
target_link_libraries(fcfs fcfs_lib)
target_link_libraries(fcfs fuse3)
#target_link_libraries(fcfs ${FUSE_LIBRARIES})
target_compile_options(fcfs PUBLIC -std=c11)
