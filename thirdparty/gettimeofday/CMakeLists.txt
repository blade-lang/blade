add_library(gettimeofday OBJECT gettimeofday.c gettimeofday.h)
target_include_directories(gettimeofday INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}")
