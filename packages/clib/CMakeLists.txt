cmake_minimum_required(VERSION 3.18)

project(clib C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)

set(CMAKE_SHARED_LIBRARY_PREFIX "")

if(WIN32 OR MINGW)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/blade)
    set(BLADE_ROOT "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
else()
    set(BLADE_ROOT "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/blade")
endif()

message(STATUS "Building clib module to ${BLADE_ROOT}/dist")

include_directories("${BLADE_ROOT}/includes" ${CMAKE_CURRENT_SOURCE_DIR})

add_library(clib SHARED clib.c)
target_link_libraries(clib libblade)
add_dependencies(clib blade)

# TEMPORARY WORKAROUND FOR THE OCCASIONAL CRASH WHEN RETURNING STRUCT
# FROM C LIBRARIES WHEN RUNNING IN RELEASE MODE.
#
# @TODO: IDENTIFY AND FIX THE REAL BUG.
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_options(clib PRIVATE -O0)
endif()

set(FFI_ROOT ${CMAKE_CURRENT_LIST_DIR}/libffi)
if(NOT WIN32)
    include(libffi/CMakeLists.txt)
    message(STATUS "FFI Version -- ${FFI_VERSION}")
    include_directories(${FFI_INCLUDE_DIRS})
    target_link_libraries(clib ${FFI_LIBRARIES})
else()
    find_package(libffi CONFIG REQUIRED)
    find_path(FFI_INCLUDE_DIR ffi.h)
    include_directories(${FFI_INCLUDE_DIR})
    target_link_libraries(clib libffi)
endif()

if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
endif()

add_custom_target(copy-clib-to-libs ALL
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/clib ${BLADE_ROOT}/libs/clib
        COMMENT "Exporting zlib source files to libs folder..."
        )

add_custom_command(TARGET clib POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:clib> ${BLADE_ROOT}/dist
        COMMENT "ZLib copy")