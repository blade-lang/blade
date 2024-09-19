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


include(FindGD.cmake)
target_include_directories(imagine PRIVATE ${GD_INCLUDE_DIR})
target_link_libraries(imagine ${GD_LIBRARIES})

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