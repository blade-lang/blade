cmake_minimum_required(VERSION 3.18...3.26)

project(imagine C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)

set(CMAKE_SHARED_LIBRARY_PREFIX "")

if(WIN32 OR MINGW)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/blade)
    set(BLADE_ROOT "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
else()
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
    set(BLADE_ROOT "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/blade")
endif()

message(STATUS "Building imagine module to ${BLADE_ROOT}/dist")

include_directories("${BLADE_ROOT}/includes" ${CMAKE_CURRENT_SOURCE_DIR})

add_library(imagine SHARED imagine.c)
target_link_libraries(imagine libblade)
add_dependencies(imagine blade)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_FIND_FRAMEWORK LAST)
set(ENABLE_CPP OFF CACHE INTERNAL "Disabled CPP")
set(ENABLE_PNG ON CACHE INTERNAL "Enabled PNG")
set(ENABLE_JPEG ON CACHE INTERNAL "Enabled JPEG")
set(BUILD_TEST OFF CACHE INTERNAL "Disabled building tests")
include(FetchContent)
FetchContent_Declare(LibGD
        URL      https://github.com/blade-lang/libgd/archive/refs/heads/master.zip
        URL_HASH SHA256=9f79756057ecff898c1e33749245435cdfd6121e8ec1e1140b00de531010e825
        )
FetchContent_MakeAvailable(LibGD)

target_include_directories(imagine PRIVATE "${libgd_SOURCE_DIR}/src")
target_link_libraries(imagine gd_static)

if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
endif()

add_custom_target(copy-imagine-to-libs ALL
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/imagine ${BLADE_ROOT}/libs/imagine
        COMMENT "Exporting imagine source files to libs folder..."
        )

add_custom_command(TARGET imagine POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:imagine> ${BLADE_ROOT}/dist
        COMMENT "imagine copy")