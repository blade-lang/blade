#ifndef bird_common_h
#define bird_common_h

// special definitions for Cygwin
#define _DEFAULT_SOURCE 1

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "config.h"

#define UINT8_COUNT (UINT8_MAX + 1)
#define UINT16_COUNT (UINT16_MAX + 1)
#define STACK_MAX (FRAMES_MAX * UINT8_COUNT)

#if defined(__unix__) || (defined(__APPLE__) && defined(__MACH__))
#define IS_UNIX
#else
#define IS_WINDOWS
#endif

#define BIRDY_EXTENSION ".b"

#endif