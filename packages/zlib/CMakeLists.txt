cmake_minimum_required(VERSION 3.18)

project(zlib2 C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)

set(CMAKE_SHARED_LIBRARY_PREFIX "")

if(WIN32 OR MINGW)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/blade)
    set(BLADE_ROOT "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
else()
    set(BLADE_ROOT "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/blade")
endif()

message(STATUS "Building zlib2 module to ${BLADE_ROOT}/dist")

include_directories("${BLADE_ROOT}/includes" ${CMAKE_CURRENT_SOURCE_DIR})
#link_directories("${BLADE_ROOT}")

add_library(zlib2 SHARED zlib.c)
target_link_libraries(zlib2 libblade)
add_dependencies(zlib2 blade)

if(NOT ${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    set(ZLIB_USE_STATIC_LIBS ON)
endif()
include(FindZLIB)
if(NOT ZLIB_FOUND)
    message(FATAL_ERROR "zlib not found!")
else()
    include_directories(${ZLIB_INCLUDE_DIRS})
    target_link_libraries(zlib2 ${ZLIB_LIBRARIES})
endif()

if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
endif()

if(WIN32)
    target_link_libraries(zlib2 wsock32 ws2_32)
endif()

add_custom_target(copy-zlib2-to-libs ALL
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/zlib ${BLADE_ROOT}/libs/zlib
        COMMENT "Exporting zlib source files to libs folder..."
        )

add_custom_command(TARGET zlib2 POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:zlib2> ${BLADE_ROOT}/dist
        COMMENT "ZLib copy")