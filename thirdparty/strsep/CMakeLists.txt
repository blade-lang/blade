add_library(openbsd-strsep strsep.c strsep.h)
target_include_directories(openbsd-strsep INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}")
