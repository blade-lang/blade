if(NOT "${HAVE_ASPRINTF}")
    add_subdirectory(asprintf)
endif()
if(NOT "${HAVE_GETTIMEOFDAY}")
    add_subdirectory(gettimeofday)
endif()
if(NOT "${HAVE_STRSEP}")
    add_subdirectory(strsep)
endif()

if(UNIX)
    add_subdirectory(linenoise)
endif()

if(WIN32)
    add_subdirectory(mman-win32)
endif()

add_subdirectory(threads)
