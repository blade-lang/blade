set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/..)

option(DISABLE_CLIB "Builds Blade without the clib module enabled" OFF)

add_subdirectory(json)
add_subdirectory(sqlite)
add_subdirectory(ssl)
add_subdirectory(curl)
add_subdirectory(zlib)
if(NOT DISABLE_CLIB)
  add_subdirectory(clib)
endif()
add_subdirectory(imagine)
add_subdirectory(bundle)
