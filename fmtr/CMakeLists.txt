cmake_minimum_required(VERSION 3.0 FATAL_ERROR)
project(fmtr VERSION 0.1 LANGUAGES C)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DDBG -D__FILENAME__='\"$(subst ${CMAKE_SOURCE_DIR}/,,$(abspath $<))\"' -g3 -Wall -lm")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

link_directories("/home/snippy/diploma/lib/lib/")
include_directories("/home/snippy/diploma/include/")

add_executable(fmtr fcfsfmt.c)
target_link_libraries(fmtr fcfs_lib)
target_compile_options(fmtr PUBLIC -std=c11)
