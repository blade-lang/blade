#ifndef BIRD_COMPAT_TIME_H
#define BIRD_COMPAT_TIME_H

#include "common.h"

#ifdef IS_UNIX

#include <sys/time.h>

#else
#include "win32.h"
#endif

#include <time.h>

#endif