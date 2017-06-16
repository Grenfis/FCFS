cmake_minimum_required(VERSION 3.0 FATAL_ERROR)
project(fcfs VERSION 0.1 LANGUAGES C)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DFUSE_USE_VERSION=31 -D__FILENAME__='\"$(subst ${CMAKE_SOURCE_DIR}/,,$(abspath $<))\"' -D_FILE_OFFSET_BITS=64 -g -Wall -lm")
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/CMake" ${CMAKE_MODULE_PATH})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

#find_package(FUSE REQUIRED)

#include_directories(${FUSE_INCLUDE_DIR})
include_directories("/home/snippy/diploma/include/")
link_directories("/home/snippy/diploma/lib/lib/")

set(CACHE
    cache/cache.c cache/file_id.c cache/file_seq.c)

set(DEV
    dev/common.c dev/dir.c dev/file.c dev/mount.c dev/read.c
    dev/write_structs.c dev/write.c dev/table.c dev/ctable.c)

set(OPS
    ops/getattr.c ops/main.c ops/mkdir.c ops/open.c ops/read.c
    ops/readdir.c ops/rmdir.c ops/create.c ops/unlink.c ops/chown.c
    ops/truncate.c ops/utimens.c ops/write.c ops/flush.c ops/opendir.c
    ops/releasedir.c ops/release.c)

set(HASHMAP
    hashmap/hashmap.c)

add_executable(fcfs fcfs.c ${OPS} ${DEV} ${HASHMAP} ${CACHE})

target_link_libraries(fcfs fcfs_lib)
target_link_libraries(fcfs fuse3)
target_link_libraries(fcfs gcrypt)
target_compile_options(fcfs PUBLIC -std=c11)
