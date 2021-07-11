cmake_minimum_required(VERSION 3.18)

if(MSVC)
  set(CMAKE_SYSTEM_VERSION "10.0.20348.0" CACHE STRING INTERNAL FORCE)
  set(CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION "10.0.20348.0" CACHE STRING INTERNAL FORCE)
endif()

project(bird)

# required modules for our task
include(CheckIncludeFile)
include(CheckIncludeFiles)
include(ExternalProject)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)

set(cfsp " ")
set(SRC_DIR ${CMAKE_CURRENT_BINARY_DIR}/..)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_CURRENT_BINARY_DIR}/bin")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG ${CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG})

# set dependency directories
set(PCRE2_DIR ${CMAKE_CURRENT_BINARY_DIR}/pcre2-build)
set(ZLIB_DIR ${CMAKE_CURRENT_BINARY_DIR}/zlib-build)

message(STATUS "Adding external library: pcre2...")
ExternalProject_Add(pcre2
    SOURCE_DIR ${SRC_DIR}/deps/pcre2-8
    PREFIX ${PCRE2_DIR}
    CMAKE_ARGS -DCMAKE_BUILD_TYPE:STRING=Release -DCMAKE_INSTALL_PREFIX:PATH=${PCRE2_DIR}/build
)

message(STATUS "Adding external library: zlib...")
ExternalProject_Add(zlib
    SOURCE_DIR ${SRC_DIR}/deps/zlib
    PREFIX ${ZLIB_DIR}
    CMAKE_ARGS -DCMAKE_BUILD_TYPE:STRING=Release -DCMAKE_INSTALL_PREFIX:PATH=${ZLIB_DIR}/build
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
    src/standard/hash.c
)

add_library(libbird ${BIRD_SOURCES})
add_executable(bird src/bird.c)

#if(MSVC)
#  set(CMAKE_DEBUG_POSTFIX
#      $<$<CONFIG:Debug>:d>
#      $<$<CONFIG:RelWithDebInfo>:>
#      $<$<CONFIG:Release>:>
#      $<$<CONFIG:MinSizeRel>:>
#      )
#else()
	set(CMAKE_DEBUG_POSTFIX "")
#endif()

set(LIBRARY_NAME_END "${CMAKE_DEBUG_POSTFIX}${CMAKE_STATIC_LIBRARY_SUFFIX}")

# set dependency shared lib path
set(PCRE2_LIB_FILE ${PCRE2_DIR}/build/lib/${CMAKE_STATIC_LIBRARY_PREFIX}pcre2-8${LIBRARY_NAME_END})

if(UNIX)
	set(ZLIB_LIB_FILE ${ZLIB_DIR}/build/lib/${CMAKE_STATIC_LIBRARY_PREFIX}z${LIBRARY_NAME_END})
else()
	set(ZLIB_LIB_FILE ${ZLIB_DIR}/build/lib/${CMAKE_STATIC_LIBRARY_PREFIX}zlib${LIBRARY_NAME_END})
endif()

# add dependency includes
include_directories(src ${PCRE2_DIR}/build/include ${ZLIB_DIR}/build/include)

add_custom_target(bird_files)

# add dependencies
add_dependencies(libbird pcre2 zlib bird_files)

set_target_properties(bird
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG}
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG}
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG}
)

set_target_properties(libbird
    PROPERTIES
    OUTPUT_NAME bird
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG}
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG}
)

target_link_libraries(libbird ${PCRE2_LIB_FILE})
target_link_libraries(libbird ${ZLIB_LIB_FILE})

if(UNIX)
  target_link_libraries(libbird readline)
  target_link_libraries(libbird m)
endif()

target_link_libraries(bird libbird)

add_custom_command(TARGET bird_files POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${SRC_DIR}/libs ${CMAKE_CURRENT_BINARY_DIR}/bin/libs
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${SRC_DIR}/tests ${CMAKE_CURRENT_BINARY_DIR}/bin/tests
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${SRC_DIR}/benchmarks ${CMAKE_CURRENT_BINARY_DIR}/bin/benchmarks
)

string(TOUPPER "${CMAKE_BUILD_TYPE}" buildtype)

string(TOUPPER "${CMAKE_BUILD_TYPE}" buildtype)
message(STATUS "Bird configuration summary")
message(STATUS "  Install prefix .................. : ${CMAKE_INSTALL_PREFIX}")
message(STATUS "  C compiler ...................... : ${CMAKE_C_COMPILER}")
message(STATUS "  Build type ...................... : ${buildtype}")
message(STATUS "  C compiler flags ................ : ${${CMAKE_C_FLAGS}${cfsp}}${CMAKE_C_FLAGS_${buildtype}}")

# for LTO
include(CheckIPOSupported)
check_ipo_supported(RESULT supported OUTPUT error)
if(supported)
  message(STATUS "  IPO/LTO ......................... : enabled")
  set_property(TARGET libbird PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
else()
  message(STATUS "  IPO/LTO ......................... : not supported: <${error}>")
endif()

enable_testing()

# define a function to simplify adding tests
function(do_test target arg index result)
  message(STATUS "setting up test ${CMAKE_CURRENT_BINARY_DIR}/bin/${PROJECT_NAME} tests/${arg}.b")
  add_test(NAME ${arg}_test_${index} COMMAND ${CMAKE_CURRENT_BINARY_DIR}/bin/${PROJECT_NAME} bin/tests/${arg}.b)
  set_tests_properties(${arg}_test_${index}
      PROPERTIES PASS_REGULAR_EXPRESSION ${result}
  )
endfunction(do_test)

# do a bunch of result based tests
do_test(bird anonymous 0 "works")
do_test(bird anonymous 1 "is the best")
do_test(bird assert 0 "AssertionError")
do_test(bird assert 1 "empty list expected")
do_test(bird bytes 0 "\\(0 0 0 0 0\\)")
do_test(bird bytes 1 "HELLO")
do_test(bird class 0 "3")
do_test(bird class 1 "10")
do_test(bird class 2 "scone with berries and cream")
do_test(bird class 3 "Person is shouting")
do_test(bird class 4 "2001")
do_test(bird class 5 "Richard. You are 15")
do_test(bird class 6 "Jane. You are 25")
do_test(bird class 7 "A")
do_test(bird class 8 "Name is set")
do_test(bird class 9 "cannot call private method '_echo'")
do_test(bird closure 0 "outer\nreturn from outer\ncreate inner closure\nvalue\n1499998500000")
do_test(bird condition 0 "Test passed\nTest passed")
do_test(bird dictionary 0 "age: 28")
do_test(bird dictionary 1 "Plot 10,")
do_test(bird dictionary 2 "30")
do_test(bird dictionary 3 "children: 2")
do_test(bird die 0 "Exception")
do_test(bird for 0 "address = Nigeria")
do_test(bird for 1 "1 = 7")
do_test(bird for 2 "n\na\nm\ne")
do_test(bird for 3 "12\n13\n14\n15")
do_test(bird for 4 "Richard\nAlex\nJustina")
do_test(bird function 0 "outer")
do_test(bird function 1 "<function test at 0x7")
do_test(bird function 2 "It works! inner")
do_test(bird function 3 "Richard")
do_test(bird function 4 "\\[James\\]")
do_test(bird function 5 "Sin 10 = -0.5440211108893656")
do_test(bird if 0 "It works")
do_test(bird if 1 "Nope")
do_test(bird if 2 "2 is less than 5")
do_test(bird if 3 "Ok")
do_test(bird import 0 "0 iteration")
do_test(bird import 1 "The new x = 0")
do_test(bird import 2 "Jane. You")
do_test(bird iter 0 "The new x = 0")
do_test(bird list 0 "\\[\\[1, 2, 4], \\[4, 5, 6\\], \\[7, 8, 9\\]\\]")
do_test(bird logarithm 0 "3.044522437723423\n3.044522437723423")
do_test(bird native 0 "10")
do_test(bird native 1 "300")
do_test(bird native 2 "\\[1, 2, 3\\]")
do_test(bird native 3 "{name: Richard, age: 28}")
do_test(bird native 4 "A class called A")
do_test(bird native 5 "9227465\nTime taken")
do_test(bird native 6 "1548008755920\nTime taken")
do_test(bird pi 0 "3.141592653589734")
do_test(bird scope 1 "inner\nouter")
do_test(bird string 0 "25, This is john's LAST 20")
do_test(bird try 0 "string index 10 out of range")
do_test(bird using 0 "ten\nafter")
do_test(bird var 0 "it works\n20\ntrue")
do_test(bird while 0 "x = 51")
