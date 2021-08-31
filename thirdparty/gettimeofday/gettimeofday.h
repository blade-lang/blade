#ifndef GETTIMEOFDAY_H
#define GETTIMEOFDAY_H

#ifdef _WIN32
#include <sdkddkver.h>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <WinSock2.h>
#endif /* ifdef _WIN32 */

int gettimeofday(struct timeval *tp, struct timezone *tzp);

#endif GETTIMEOFDAY_H
