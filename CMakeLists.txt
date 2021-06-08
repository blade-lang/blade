cmake_minimum_required(VERSION 3.18)
project(bird)

# required modules for our task
include(CheckIncludeFile)
include(CheckIncludeFiles)
include(ExternalProject)

set(CMAKE_C_STANDARD 11)

set(SRC_DIR ${CMAKE_CURRENT_BINARY_DIR}/..)

# set dependency directories
set(PCRE2_DIR ${CMAKE_CURRENT_BINARY_DIR}/pcre2-build)

message("Adding external library: pcre2...")
ExternalProject_Add(pcre2
    SOURCE_DIR ${SRC_DIR}/deps/pcre2-8
    PREFIX ${PCRE2_DIR}
    CMAKE_ARGS -DCMAKE_BUILD_TYPE:STRING=Release -DCMAKE_INSTALL_PREFIX:PATH=${PCRE2_DIR}/build
)

set(BIRD_SOURCES
    src/b_dict.c
    src/b_file.c
    src/b_getopt.c
    src/b_list.c
    src/b_string.c
    src/blob.c
    src/bytes.c
    src/compiler.c
    src/debug.c
    src/memory.c
    src/module.c
    src/native.c
    src/object.c
    src/pathinfo.c
    src/scanner.c
    src/table.c
    src/util.c
    src/value.c
    src/win32.c
    src/vm.c
    # standard module
    src/standard/base64.c
    src/standard/date.c
    src/standard/io.c
    src/standard/math.c
    src/standard/os.c
    src/standard/socket.c
)

add_library(libbird ${BIRD_SOURCES})
add_executable(bird src/bird.c)

# set dependency shared lib path
set(PCRE2_LIB_FILE ${PCRE2_DIR}/build/lib/${CMAKE_STATIC_LIBRARY_PREFIX}pcre2-8${CMAKE_STATIC_LIBRARY_SUFFIX})

# add dependency includes
include_directories(src ${PCRE2_DIR}/build/include)

# add dependencies
add_dependencies(libbird pcre2)

set_target_properties(bird
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/bin"
)

set_target_properties(libbird
    PROPERTIES
    OUTPUT_NAME bird
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/bin"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/bin"
)

target_link_libraries(libbird ${PCRE2_LIB_FILE})

if(UNIX)
  target_link_libraries(libbird readline)
  target_link_libraries(libbird m)
endif(UNIX)

target_link_libraries(bird libbird)

if(RUNTIME_OUTPUT_DIRECTORY)
  add_custom_command(TARGET bird POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_directory ${SRC_DIR}/libs ${RUNTIME_OUTPUT_DIRECTORY}/libs
      )
else()
  add_custom_command(TARGET bird POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_directory ${SRC_DIR}/libs ${CMAKE_CURRENT_BINARY_DIR}/libs
      )
endif(RUNTIME_OUTPUT_DIRECTORY)

