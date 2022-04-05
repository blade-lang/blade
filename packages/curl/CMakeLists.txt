cmake_minimum_required(VERSION 3.18)

project(curl C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)

set(CMAKE_SHARED_LIBRARY_PREFIX "")
set(BLADE_ROOT "${CMAKE_CURRENT_BINARY_DIR}/../../blade")

message(STATUS "Building curl module to ${CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE}")

include_directories("${BLADE_ROOT}/includes" ${CMAKE_CURRENT_SOURCE_DIR})
#link_directories("${BLADE_ROOT}")

add_library(curl SHARED curl.c)
target_link_libraries(curl libblade)
add_dependencies(curl blade)

include(FindCURL)
if(${CURL_FOUND})
    include_directories(${CURL_INCLUDE_DIRS})
    target_link_libraries(curl ${CURL_LIBRARIES})
else()
    message(FATAL_ERROR "cURL not found!")
endif()

if(UNIX)
    target_link_libraries(curl m)
endif()

if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
endif()

if(WIN32)
    target_link_libraries(curl wsock32 ws2_32)
endif()

add_custom_target(copy-curl-to-libs ALL
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/curl ${CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE}/../libs/curl
        COMMENT "Exporting curl source files to libs folder..."
        )