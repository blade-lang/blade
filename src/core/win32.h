#ifndef bird_compat_win32_h
#define bird_compat_win32_h

#include "common.h"

#ifdef _WIN32

#include <windows.h>
#include <winbase.h>
#include <_timeval.h>
#include <time.h>

#define sigjmp_buf jmp_buf
#define siglongjmp longjmp
#define sigsetjmp(a,b) setjmp(a)
#define lstat stat
#define S_ISLNK S_ISBLK

int gettimeofday(struct timeval *time_Info, struct timezone *timezone_Info);

#endif

#endif