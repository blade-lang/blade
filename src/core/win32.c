#include "common.h"
#include "util.h"

#ifdef IS_WINDOWS

#include "win32.h"
#include <sys/timeb.h>
#include <lm.h>

const char* GetWindowsVersionString()
{
    const char* winver = NULL;
    OSVERSIONINFOEXW osver;
    SYSTEM_INFO     sysInfo;

#ifndef __MINGW32_MAJOR_VERSION
    __pragma(warning(push))
        __pragma(warning(disable:4996))
#endif
        memset(&osver, 0, sizeof(osver));
    osver.dwOSVersionInfoSize = sizeof(osver);
    GetVersionExW((LPOSVERSIONINFOW)&osver);

#ifndef __MINGW32_MAJOR_VERSION
    __pragma(warning(pop))
#endif

        if (osver.dwMajorVersion == 6 && osver.dwMinorVersion == 2)
        {
            OSVERSIONINFOEXW osvi = osver;
            ULONGLONG cm = 0;
            cm = VerSetConditionMask(cm, VER_MINORVERSION, VER_EQUAL);
            osvi.dwOSVersionInfoSize = sizeof(osvi);
            osvi.dwMinorVersion = 3;
            if (VerifyVersionInfoW(&osvi, VER_MINORVERSION, cm))
            {
                osver.dwMinorVersion = 3;
            }
        }

    GetSystemInfo(&sysInfo);

    if (osver.dwMajorVersion == 10 && osver.wProductType != VER_NT_WORKSTATION)  winver = "Windows 10 Server";
    else if (osver.dwMajorVersion == 10 && osver.wProductType == VER_NT_WORKSTATION)  winver = "Windows 10";
    else if (osver.dwMajorVersion == 6 && osver.dwMinorVersion == 3 && osver.wProductType != VER_NT_WORKSTATION)  winver = "Windows Server 2012 R2";
    else if (osver.dwMajorVersion == 6 && osver.dwMinorVersion == 3 && osver.wProductType == VER_NT_WORKSTATION)  winver = "Windows 8.1";
    else if (osver.dwMajorVersion == 6 && osver.dwMinorVersion == 2 && osver.wProductType != VER_NT_WORKSTATION)  winver = "Windows Server 2012";
    else if (osver.dwMajorVersion == 6 && osver.dwMinorVersion == 2 && osver.wProductType == VER_NT_WORKSTATION)  winver = "Windows 8";
    else if (osver.dwMajorVersion == 6 && osver.dwMinorVersion == 1 && osver.wProductType != VER_NT_WORKSTATION)  winver = "Windows Server 2008 R2";
    else if (osver.dwMajorVersion == 6 && osver.dwMinorVersion == 1 && osver.wProductType == VER_NT_WORKSTATION)  winver = "Windows 7";
    else if (osver.dwMajorVersion == 6 && osver.wProductType != VER_NT_WORKSTATION)  winver = "Windows Server 2008";
    else if (osver.dwMajorVersion == 6 && osver.wProductType == VER_NT_WORKSTATION)  winver = "Windows Vista";
    else if (osver.dwMajorVersion == 5 && osver.dwMinorVersion == 2 && osver.wProductType == VER_NT_WORKSTATION
        && sysInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64)  winver = "Windows XP x64";
    else if (osver.dwMajorVersion == 5 && osver.dwMinorVersion == 2)   winver = "Windows Server 2003";
    else if (osver.dwMajorVersion == 5 && osver.dwMinorVersion == 1)   winver = "Windows XP";
    else if (osver.dwMajorVersion == 5 && osver.dwMinorVersion == 0)   winver = "Windows 2000";
    else winver = "unknown";
    return winver;
}

int uname(struct utsname* sys) {
    // sys
    strncpy(sys->sysname, "Windows", 8);

    // get system version
    WORD arch = 0;
    const char* sysname = GetWindowsVersionString(&arch);
    memcpy(sys->version, sysname, (int)strlen(sysname));
    memcpy(sys->release, sysname, (int)strlen(sysname));

    // Get computer name
    DWORD cc_buffer_size = MAX_COMPUTERNAME_LENGTH + 1;
    if (!GetComputerNameA(sys->nodename, &cc_buffer_size)) {
        return 1;
    }

    // Set machine
    switch (arch) {
    case PROCESSOR_ARCHITECTURE_INTEL:
    case PROCESSOR_AMD_X8664: {
        strncpy(sys->machine, "x86_64", 7);
        break;
    }
    case PROCESSOR_ARCHITECTURE_AMD64: {
        strncpy(sys->machine, "amd64", 6);
        break;
    }
    case PROCESSOR_ARCHITECTURE_ARM: {
        strncpy(sys->machine, "arm", 4);
        break;
    }
    default: {
        strncpy(sys->machine, "unknown", 8);
        break;
    }
    }

    return 0;
}

int gettimeofday(struct timeval *time_info, struct timezone *timezone_info) {

   if (time_info != NULL) {

       uint64_t UNIX_TIME_START = 116444736000000000Ui64; //January 1, 1970 (start of Unix epoch) in "ticks"
       uint64_t TICKS_PER_SECOND = 10000000Ui64; //a tick is 100ns

       FILETIME ft;
       GetSystemTimeAsFileTime(&ft); //returns ticks in UTC

       //Copy the low and high parts of FILETIME into a LARGE_INTEGER
       //This is so we can access the full 64-bits as an Int64 without causing an alignment fault
       LARGE_INTEGER li;
       li.LowPart = ft.dwLowDateTime;
       li.HighPart = ft.dwHighDateTime;

       uint64_t tm = li.QuadPart - UNIX_TIME_START;

       //Convert ticks since 1/1/1970 into seconds
       time_info->tv_sec = tm / TICKS_PER_SECOND;
       time_info->tv_usec = (long)(tm % TICKS_PER_SECOND);
   }

   if (timezone_info != NULL) {
       _tzset();
       timezone_info->tz_minuteswest = _timezone;
       timezone_info->tz_dsttime = _daylight;
   }

  return 0;
}

char* dirname(char* path) {
    char* drive = (char*)malloc(sizeof(char));
    char* dir = (char*)malloc(sizeof(char) * MAX_PATH);
    _splitpath((const char*)path, drive, dir, NULL, NULL);
    return append_strings(drive, dir);
}

#endif