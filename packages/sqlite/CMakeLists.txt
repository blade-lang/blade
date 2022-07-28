cmake_minimum_required(VERSION 3.18)

project(sqlite C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)

set(CMAKE_SHARED_LIBRARY_PREFIX "")
set(BLADE_ROOT "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/blade")

message(STATUS "Building SQLite module to ${BLADE_ROOT}/dist")

include_directories("${BLADE_ROOT}/includes" ${CMAKE_CURRENT_SOURCE_DIR})
#link_directories("${BLADE_ROOT}")

add_library(sqlite SHARED sqlite3.c sqlite3.h sqlite.c)
target_link_libraries(sqlite libblade)
add_dependencies(sqlite blade)

if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
    target_link_libraries(sqlite pthread)
endif()

if(WIN32)
    target_link_libraries(sqlite wsock32 ws2_32)
endif()

add_custom_target(copy-sqlite-to-libs ALL
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/sqlite ${BLADE_ROOT}/libs/sqlite
        COMMENT "Exporting sqlite source files to libs folder..."
        )

add_custom_command(TARGET sqlite POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:sqlite> ${BLADE_ROOT}/dist)