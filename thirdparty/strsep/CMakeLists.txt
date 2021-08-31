add_library(strsep strsep.c strsep.h)
target_include_directories(strsep INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}")
