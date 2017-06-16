cmake_minimum_required(VERSION 3.0 FATAL_ERROR)
project(fcfs_lib VERSION 0.2 LANGUAGES C)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DDBG -D__FILENAME__='\"$(subst ${CMAKE_SOURCE_DIR}/,,$(abspath $<))\"' -D_FILE_OFFSET_BITS=64 -g -Wall")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

add_library(fcfs_lib STATIC scsi.c utils.c)
target_compile_options(fcfs_lib PUBLIC -std=c11)

include_directories("/home/snippy/diploma/include/")
