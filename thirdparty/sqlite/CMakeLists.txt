add_library(sqlite STATIC sqlite3.c sqlite3.h)
target_include_directories(sqlite INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}")

if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC"  )
endif()
