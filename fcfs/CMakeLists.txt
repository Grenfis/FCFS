cmake_minimum_required(VERSION 3.0 FATAL_ERROR)
project(fcfs VERSION 0.2 LANGUAGES C)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DFUSE_USE_VERSION=31 -D__FILENAME__='\"$(subst ${CMAKE_SOURCE_DIR}/,,$(abspath $<))\"' -D_FILE_OFFSET_BITS=64 -g -Wall -lm")
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/CMake" ${CMAKE_MODULE_PATH})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

include_directories("/home/snippy/diploma/include/")
link_directories("/home/snippy/diploma/lib/lib/")

set(CACHE
    src/cache/cache.c src/cache/file_id.c src/cache/file_seq.c)

set(DEV
    src/dev/common.c src/dev/dir.c src/dev/file.c src/dev/mount.c src/dev/read.c
    src/dev/unmount.c src/dev/write.c src/dev/table.c src/dev/ctable.c)

set(OPS
    src/ops/getattr.c src/ops/main.c src/ops/mkdir.c src/ops/open.c src/ops/read.c
    src/ops/readdir.c src/ops/rmdir.c src/ops/create.c src/ops/unlink.c src/ops/chown.c
    src/ops/truncate.c src/ops/utimens.c src/ops/write.c src/ops/flush.c src/ops/opendir.c
    src/ops/releasedir.c src/ops/release.c)

set(HASHMAP
    hashmap/hashmap.c)

add_executable(fcfs src/fcfs.c ${OPS} ${DEV} ${HASHMAP} ${CACHE})

target_link_libraries(fcfs fcfs_lib)
target_link_libraries(fcfs fuse3)
target_link_libraries(fcfs gcrypt)
target_compile_options(fcfs PUBLIC -std=c11)
