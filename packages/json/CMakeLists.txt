cmake_minimum_required(VERSION 3.18)

project(json C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)

set(CMAKE_SHARED_LIBRARY_PREFIX "")
set(BLADE_ROOT "${CMAKE_CURRENT_BINARY_DIR}/../../blade")

message(STATUS "Building JSON module to ${CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE}")

include_directories("${BLADE_ROOT}/includes" ${CMAKE_CURRENT_SOURCE_DIR})
#link_directories("${BLADE_ROOT}")

add_library(json SHARED json-parser.c json.h json.c)
target_link_libraries(json libblade)
add_dependencies(json blade)

if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
endif()

if(WIN32)
    target_link_libraries(json wsock32 ws2_32)
endif()

add_custom_target(copy-json-to-libs ALL
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/json ${CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE}/../libs/json
        COMMENT "Exporting json source files to libs folder..."
        )