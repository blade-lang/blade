add_library(mman OBJECT mman.c mman.h)
target_include_directories(mman INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}")