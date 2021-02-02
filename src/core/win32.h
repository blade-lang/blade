#ifndef bird_compat_win32_h
#define bird_compat_win32_h

#include "common.h"

#ifdef IS_WINDOWS

#include <winbase.h>
#include <windows.h>

int gettimeofday(struct timeval *time_Info, struct timezone *timezone_Info)

#endif

#endif