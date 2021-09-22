add_library(sqlite STATIC sqlite3.c sqlite3.h)
target_include_directories(sqlite INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}")

if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC"  )
endif()

if(WIN32)
    target_compile_definitions(sqlite PUBLIC sqlite)
	generate_export_header(sqlite)
    set_target_properties(sqlite PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)
endif()
