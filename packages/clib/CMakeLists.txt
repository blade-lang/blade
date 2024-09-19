cmake_minimum_required(VERSION 3.18...3.26)

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
# @TODO: IDENTIFY AND FIX THE REAL BUG THAT CAUSES SEGFAULT IN RELEASE BUILD.
#        THE BUG IS RELATED TO ASLR. IT IS STILL UNCLEAR IF THIS BUG ONLY EXISTS
#        UNDER CERTAIN OS CONFIGURATIONS.
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_options(clib PRIVATE -O0)
endif()

if(NOT WIN32)
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
    include(FetchContent)
    FetchContent_Declare(LibFFI
            URL      https://github.com/blade-lang/ffi/archive/refs/heads/main.zip
            URL_HASH SHA256=75bd74be525e74112a1c84da6648b9e995d285200ac3354eab59f341bd78dbb8
            )
    FetchContent_MakeAvailable(LibFFI)
    set(FFI_ROOT ${libffi_BINARY_DIR})

    target_include_directories(clib PRIVATE "${FFI_ROOT}/include")
    target_link_libraries(clib libffi)
else()
    find_package(PkgConfig)
    find_path(FFI_INCLUDE_DIR ffi.h)
    pkg_check_modules(LIBFFI REQUIRED IMPORTED_TARGET libffi)
    target_link_libraries(main PRIVATE PkgConfig::LIBFFI)
endif()

if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
endif()

add_custom_target(copy-clib-to-libs ALL
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/clib ${BLADE_ROOT}/libs/clib
        COMMENT "Exporting clib source files to libs folder..."
        )

add_custom_command(TARGET clib POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:clib> ${BLADE_ROOT}/dist
        COMMENT "clib copy")