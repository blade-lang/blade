add_library(json-parser OBJECT json.c json.h)
target_include_directories(json-parser INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}")
