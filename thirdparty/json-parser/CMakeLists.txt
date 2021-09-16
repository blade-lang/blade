add_library(json-parser json.c json.h)
target_include_directories(json-parser INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}")
