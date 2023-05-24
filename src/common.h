#ifndef BLADE_COMMON_H
#define BLADE_COMMON_H

// special definitions for Cygwin
#define _DEFAULT_SOURCE 1
#define _GNU_SOURCE 1
#define _ln_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>



#ifdef __GNUC__
#define UNUSED __attribute__((unused))
#elif defined(_MSC_VER)
#define UNUSED __pragma(warning(suppress: 4100 4101 4189))
#else
    #define UNUSED
#endif

#ifdef __GNUC__
#define DISABLE_WARNING_PUSH _Pragma("GCC diagnostic push")
#define DISABLE_WARNING_POP _Pragma("GCC diagnostic pop")
#define DISABLE_WARNING(w) _Pragma(GCC diagnostic ignored #w)
#elif defined(_MSC_VER)
#define DISABLE_WARNING_PUSH __pragma(warning(push))
    #define DISABLE_WARNING_POP __pragma(warning(pop))
    #define DISABLE_WARNING(w) __pragma(warning(disable:w))
#else
    #define DISABLE_WARNING_PUSH
    #define DISABLE_WARNING_POP
    #define DISABLE_WARNING(w)
#endif

#include "config.h"
#include "bendian.h"

// --> debug mode options starts here...
#if DEBUG_MODE == 1

#define DEBUG_PRINT_CODE 1
#define DEBUG_TABLE 1
#define DEBUG_GC 1
#define DEBUG_STACK 0

#endif
// --> debug mode options ends here...

#define UINT8_COUNT (UINT8_MAX + 1)
#define UINT16_COUNT (UINT16_MAX + 1)
#define STACK_MAX (FRAMES_MAX * UINT8_COUNT)

#if defined(__unix__) || (defined(__APPLE__) && defined(__MACH__))
#define IS_UNIX
#elif defined _WIN32
// #define NO_OLDNAMES
#endif

#define VERSION(x) #x
#define VERSION_STRING(name, major, minor, patch)                              \
  name " " VERSION(major) "." VERSION(minor) "." VERSION(patch)

#ifdef __clang__

#define COMPILER                                                               \
  VERSION_STRING("Clang", __clang_major__, __clang_minor__,                    \
                 __clang_patchlevel__)

#elif defined(_MSC_VER)

#define COMPILER VERSION_STRING("MSC", _MSC_VER, 0, 0)

#elif defined(__MINGW32_MAJOR_VERSION)

#define COMPILER                                                               \
  VERSION_STRING("MinGW32", __MINGW32_MAJOR_VERSION, __MINGW32_MINOR_VERSION, 0)

#elif defined(__MINGW64_VERSION_MAJOR)

#define COMPILER                                                               \
  VERSION_STRING("MinGW-64", __MINGW64_VERSION_MAJOR, __MINGW64_VERSION_MAJOR, \
                 0)

#elif defined(__GNUC__)

#define COMPILER                                                               \
  VERSION_STRING("GCC", __GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__)

#else

#define COMPILER "Unknown Compiler"

#endif

#if defined(__APPLE__) && defined(__MACH__) // Apple OSX and iOS (Darwin)
#define LIBRARY_FILE_EXTENSION ".dylib"
#elif defined(_WIN32)
#define LIBRARY_FILE_EXTENSION ".dll"
#else
#define LIBRARY_FILE_EXTENSION ".so"
#endif

#define DEFAULT_GC_START (1024 * 1024)


#define EXIT_COMPILE 10
#define EXIT_RUNTIME 11
#define EXIT_TERMINAL 12

#define BLADE_COPYRIGHT "Copyright (c) 2021 - 2023 Ore Richard Muyiwa"

#endif
