cmake_minimum_required(VERSION 3.18)

project(ssl C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)

set(CMAKE_SHARED_LIBRARY_PREFIX "")
set(BLADE_ROOT "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/blade")

message(STATUS "Building ssl module to ${BLADE_ROOT}/dist")

include_directories("${BLADE_ROOT}/includes" ${CMAKE_CURRENT_SOURCE_DIR})

add_library(ssl SHARED ssl.h ssl.c)
target_link_libraries(ssl libblade)
add_dependencies(ssl blade)

if(NOT ${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    set(OPENSSL_USE_STATIC_LIBS TRUE)
endif()

find_package(OpenSSL REQUIRED)
if(NOT OPENSSL_FOUND)
    message(FATAL_ERROR "OpenSSL not found!")
else()
    include_directories(${OPENSSL_INCLUDE_DIR})
    target_link_libraries(ssl OpenSSL::SSL OpenSSL::Crypto)
endif()

if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
endif()

if(WIN32)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-deprecated-declarations")
endif()

add_custom_target(copy-ssl-to-libs ALL
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/ssl ${BLADE_ROOT}/libs/ssl
        COMMENT "Exporting ssl source files to libs folder..."
        )

add_custom_command(TARGET ssl POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:ssl> ${BLADE_ROOT}/dist)