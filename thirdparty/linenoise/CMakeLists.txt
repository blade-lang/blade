add_library(linenoise OBJECT linenoise.c linenoise.c utf8.c utf8.h)
target_include_directories(linenoise INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}")
