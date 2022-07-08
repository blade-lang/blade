cmake_minimum_required(VERSION 3.18)

project(zlib C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)

set(CMAKE_SHARED_LIBRARY_PREFIX "")
set(BLADE_ROOT "${CMAKE_CURRENT_BINARY_DIR}/../../blade")

message(STATUS "Building zlib module to ${CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE}")

include_directories("${BLADE_ROOT}/includes" ${CMAKE_CURRENT_SOURCE_DIR})
#link_directories("${BLADE_ROOT}")

add_library(zlib SHARED zlib.c)
target_link_libraries(zlib libblade)
add_dependencies(zlib blade)

if(NOT ${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    set(ZLIB_USE_STATIC_LIBS ON)
endif()
include(FindZLIB)
if(NOT ZLIB_FOUND)
    message(FATAL_ERROR "zlib not found!")
else()
    include_directories(${ZLIB_INCLUDE_DIRS})
    target_link_libraries(zlib ${ZLIB_LIBRARIES})
endif()

if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
endif()

if(WIN32)
    target_link_libraries(zlib wsock32 ws2_32)
endif()

add_custom_target(copy-zlib-to-libs ALL
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/zlib ${CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE}/../libs/zlib
        COMMENT "Exporting zlib source files to libs folder..."
        )