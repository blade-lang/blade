#ifndef bird_time_h
#define bird_time_h

#include <time.h>

#ifdef IS_UNIX
#include <sys/time.h>
#else
#include "win32.h"
#endif

#endif