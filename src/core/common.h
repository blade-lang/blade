#ifndef bird_common_h
#define bird_common_h

// special definitions for Cygwin
#define _DEFAULT_SOURCE 1
#define _ln_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "config.h"

// --> debug mode options starts here...
#if DEBUG_MODE == 1

#define DEBUG_TRACE_EXECUTION 0
#define DEBUG_PRINT_CODE 1
#define DEBUG_TABLE 0
#define DEBUG_STRESS_GC 1
#define DEBUG_LOG_GC 0

#endif
// --> debug mode options ends here...

#define UINT8_COUNT (UINT8_MAX + 1)
#define UINT16_COUNT (UINT16_MAX + 1)
#define STACK_MAX (FRAMES_MAX * UINT8_COUNT)

#if defined(__unix__) || (defined(__APPLE__) && defined(__MACH__))
#define IS_UNIX
#elif defined _WIN32
#define IS_WINDOWS
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

#define COMPILER VERSION_STRING("Microsoft Visual C++", _MSC_VER)

#elif defined(__MINGW32_MAJOR_VERSION)

#define COMPILER                                                               \
  VERSION_STRING("MinGW32", __MINGW32_MAJOR_VERSION, __MINGW32_MINOR_VERSION, 0)

#elif defined(__MINGW64_VERSION_MAJOR)

#define COMPILER                                                               \
  VERSION_STRING("MinGW-64", __MINGW64_VERSION_MAJOR, __MINGW64_VERSION_MAJOR, 0)

#elif defined(__GNUC__)

#define COMPILER                                                               \
  VERSION_STRING("GCC", __GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__)

#else

#define COMPILER "Unknown Compiler"

#endif

#define BIRD_EXTENSION ".b"
#define BIRD_VERSION_STRING "0.0.1"
#define BVM_VERSION "0.0.1"
#define BIRD_VERSION_NUMBER 1
#define BVM_VERSION_NUMBER 1

#endif