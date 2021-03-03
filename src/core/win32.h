#ifndef bird_compat_win32_h
#define bird_compat_win32_h

#include "common.h"

#ifdef _WIN32

#include <time.h>
// Hack to force windows.h to always appear before winbase.h
// irrespective of C formatting standard
#include <windows.h>
//--
#include <winbase.h>

/*
//struct timeval
//{
//	long tv_sec;
//	long tv_usec;
//};
//
//#define timerisset(tvp)		((tvp)->tv_sec || (tvp)->tv_usec)
//#define timercmp(tvp,uvp,cmp)					\
//		((tvp)->tv_sec cmp (uvp)->tv_sec ||		\
//		 ((tvp)->tv_sec == (uvp)->tv_sec && (tvp)->tv_usec cmp
(uvp)->tv_usec))
//#define timerclear(tvp)		(tvp)->tv_sec = (tvp)->tv_usec = 0
*/

#define _CRT_INTERNAL_NONSTDC_NAMES 1

#ifndef __MINGW32_MAJOR_VERSION
struct timezone {
  int tz_minuteswest;
  int tz_dsttime;
};
#endif

#define sigjmp_buf jmp_buf
#define siglongjmp longjmp
#define sigsetjmp(a, b) setjmp(a)
#define lstat stat
#define S_ISLNK S_ISBLK

//#define fopen fopen_s
//#define sprintf sprintf_s
#ifndef __MINGW32_MAJOR_VERSION
#define mode_t int
#endif
#define strdup _strdup

#define _UTSNAME_LENGTH 256
struct utsname {
  char sysname[_UTSNAME_LENGTH];
  char nodename[MAX_COMPUTERNAME_LENGTH + 1];
  char release[_UTSNAME_LENGTH];
  char version[_UTSNAME_LENGTH];
  char machine[_UTSNAME_LENGTH];
  char domainname[_UTSNAME_LENGTH];
};

struct utimbuf {
  time_t actime;  /* Access time */
  time_t modtime; /* Modification time */
};

int gettimeofday(struct timeval *time_info, struct timezone *timezone_info);
const char *GetWindowsVersionString();
int uname(struct utsname *sys);
char *dirname(char *path);
char *basename(char *path);

#endif

#endif