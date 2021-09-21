add_library(asprintf OBJECT vasprintf-c99.c asprintf.c asprintf.h)
target_include_directories(asprintf INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}")
set_target_properties(asprintf PROPERTIES
        C_STANDARD 99
        C_STANDARD_REQUIRED ON)