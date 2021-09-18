add_library(sqlite sqlite3.c sqlite3.h)
target_include_directories(sqlite INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}")
