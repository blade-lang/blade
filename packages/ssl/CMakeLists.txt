cmake_minimum_required(VERSION 3.18)

project(ssl C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)

set(CMAKE_SHARED_LIBRARY_PREFIX "")
set(BLADE_ROOT "${CMAKE_CURRENT_BINARY_DIR}/../../blade")

message(STATUS "Building ssl module to ${CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE}")

include_directories("${BLADE_ROOT}/includes" ${CMAKE_CURRENT_SOURCE_DIR})
#link_directories("${BLADE_ROOT}")

add_library(ssl SHARED ssl.h ssl.c)
target_link_libraries(ssl libblade)
add_dependencies(ssl blade)

if(NOT ${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    set(OPENSSL_USE_STATIC_LIBS TRUE)
endif()
include(FindOpenSSL)
if(NOT OPENSSL_FOUND)
    message(FATAL_ERROR "OpenSSL not found!")
else()
    include_directories(${OPENSSL_INCLUDE_DIR})
    target_link_libraries(ssl ${OPENSSL_SSL_LIBRARY} ${OPENSSL_CRYPTO_LIBRARY})
endif()

if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
endif()

if(WIN32)
    target_link_libraries(ssl wsock32 ws2_32)
endif()

add_custom_target(copy-ssl-to-libs ALL
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/ssl ${CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE}/../libs/ssl
        COMMENT "Exporting ssl source files to libs folder..."
        )