#include "os.h"
#include "compat/unistd.h"
#include "win32.h"

#ifdef _WIN32
#define _UTSNAME_LENGTH 256
struct utsname
{
  char sysname[_UTSNAME_LENGTH];
  char nodename[MAX_COMPUTERNAME_LENGTH + 1];
  char release[_UTSNAME_LENGTH];
  char version[_UTSNAME_LENGTH];
  char machine[_UTSNAME_LENGTH];
  char domainname[_UTSNAME_LENGTH];
};

#include <lm.h>

bool GetWinMajorMinorVersion(DWORD *major, DWORD *minor)
{
    bool bRetCode = false;
    LPBYTE pinfoRawData = 0;
    if (NERR_Success == NetWkstaGetInfo(NULL, 100, &pinfoRawData))
    {
        WKSTA_INFO_100* pworkstationInfo = (WKSTA_INFO_100*)pinfoRawData;
        *major = pworkstationInfo->wki100_ver_major;
        *minor = pworkstationInfo->wki100_ver_minor;
        NetApiBufferFree(pinfoRawData);
        bRetCode = true;
    }
    return bRetCode;
}

const char *GetWindowsVersionString(WORD *arch)
{
    const char*     winver;
    OSVERSIONINFOEX osver;
    SYSTEM_INFO     sysInfo;

    memset(&osver, 0, sizeof(osver));
    osver.dwOSVersionInfoSize = sizeof(osver);
    GetVersionEx((LPOSVERSIONINFO)&osver);

    DWORD major = 0;
    DWORD minor = 0;
    if (GetWinMajorMinorVersion(&major, &minor))
    {
        osver.dwMajorVersion = major;
        osver.dwMinorVersion = minor;
    }
    else if (osver.dwMajorVersion == 6 && osver.dwMinorVersion == 2)
    {
        OSVERSIONINFOEXW osvi;
        ULONGLONG cm = 0;
        cm = VerSetConditionMask(cm, VER_MINORVERSION, VER_EQUAL);
        ZeroMemory(&osvi, sizeof(osvi));
        osvi.dwOSVersionInfoSize = sizeof(osvi);
        osvi.dwMinorVersion = 3;
        if (VerifyVersionInfoW(&osvi, VER_MINORVERSION, cm))
        {
            osver.dwMinorVersion = 3;
        }
    }

    GetSystemInfo(&sysInfo);

    *arch = sysInfo.wProcessorArchitecture;

    if (osver.dwMajorVersion == 10 && osver.wProductType != VER_NT_WORKSTATION)  winver = "Windows 10 Server";
    if (osver.dwMajorVersion == 10 && osver.wProductType == VER_NT_WORKSTATION)  winver = "Windows 10";
    if (osver.dwMajorVersion == 6 && osver.dwMinorVersion == 3 && osver.wProductType != VER_NT_WORKSTATION)  winver = "Windows Server 2012 R2";
    if (osver.dwMajorVersion == 6 && osver.dwMinorVersion == 3 && osver.wProductType == VER_NT_WORKSTATION)  winver = "Windows 8.1";
    if (osver.dwMajorVersion == 6 && osver.dwMinorVersion == 2 && osver.wProductType != VER_NT_WORKSTATION)  winver = "Windows Server 2012";
    if (osver.dwMajorVersion == 6 && osver.dwMinorVersion == 2 && osver.wProductType == VER_NT_WORKSTATION)  winver = "Windows 8";
    if (osver.dwMajorVersion == 6 && osver.dwMinorVersion == 1 && osver.wProductType != VER_NT_WORKSTATION)  winver = "Windows Server 2008 R2";
    if (osver.dwMajorVersion == 6 && osver.dwMinorVersion == 1 && osver.wProductType == VER_NT_WORKSTATION)  winver = "Windows 7";
    if (osver.dwMajorVersion == 6 && osver.wProductType != VER_NT_WORKSTATION)  winver = "Windows Server 2008";
    if (osver.dwMajorVersion == 6  && osver.wProductType == VER_NT_WORKSTATION)  winver = "Windows Vista";
    if (osver.dwMajorVersion == 5 && osver.dwMinorVersion == 2 && osver.wProductType == VER_NT_WORKSTATION
        &&  sysInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64)  winver = "Windows XP x64";
    if (osver.dwMajorVersion == 5 && osver.dwMinorVersion == 2)   winver = "Windows Server 2003";
    if (osver.dwMajorVersion == 5 && osver.dwMinorVersion == 1)   winver = "Windows XP";
    if (osver.dwMajorVersion == 5 && osver.dwMinorVersion == 0)   winver = "Windows 2000";
    if (osver.dwMajorVersion < 5)   winver = "unknown";
    return winver;
}

int uname (struct utsname * sys) {
  // sys
  strncpy(sys->sysname, "Windows", 8);

  // get system version
  WORD arch;
  const char* sysname = GetWindowsVersionString(&arch);
  memcpy(sys->version, sysname, (int)strlen(sysname));
  memcpy(sys->release, sysname, (int)strlen(sysname));

  // Get computer name
  DWORD cc_buffer_size = MAX_COMPUTERNAME_LENGTH + 1;
  if(!GetComputerNameA(sys->nodename, &cc_buffer_size)) {
    return 1;
  }

  // Set machine
  switch(arch) {
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
#else
#include <sys/utsname.h>
#endif

#include <stdio.h>
#include <ctype.h>

DECLARE_MODULE_METHOD(os_exec) {
  ENFORCE_ARG_COUNT(exec, 1);
  ENFORCE_ARG_TYPE(exec, 0, IS_STRING);
  b_obj_string *string = AS_STRING(args[0]);
  if (string->length == 0) {
    RETURN;
  }

  FILE *fd = popen(string->chars, "r");
  if (!fd)
    RETURN;

  char buffer[256];
  size_t nread;
  size_t output_size = 256;
  int length = 0;
  char *output = malloc(output_size);

  while ((nread = fread(buffer, 1, sizeof(buffer), fd)) != 0) {
    if (length + nread >= output_size) {
      output_size *= 2;
      output = realloc(output, output_size);
    }
    strncat(output + length, buffer, nread);
    length += nread;
  }

  if(length == 0)
    RETURN;
  
  output[length - 1] = '\0';

  pclose(fd);
  RETURN_LSTRING(output, length);
}

DECLARE_MODULE_METHOD(os_info) {
  ENFORCE_ARG_COUNT(info, 0);
  struct utsname os;
  if (uname(&os) != 0) {
    RETURN_ERROR("could not access os information");
  }

  b_obj_dict *dict = new_dict(vm);
  dict_add_entry(vm, dict, OBJ_VAL(copy_string(vm, "sysname", 7)),
                 OBJ_VAL(copy_string(vm, os.sysname, strlen(os.sysname))));
  dict_add_entry(vm, dict, OBJ_VAL(copy_string(vm, "nodename", 8)),
                 OBJ_VAL(copy_string(vm, os.nodename, strlen(os.nodename))));
  dict_add_entry(vm, dict, OBJ_VAL(copy_string(vm, "version", 7)),
                 OBJ_VAL(copy_string(vm, os.version, strlen(os.version))));
  dict_add_entry(vm, dict, OBJ_VAL(copy_string(vm, "release", 7)),
                 OBJ_VAL(copy_string(vm, os.release, strlen(os.release))));
  dict_add_entry(vm, dict, OBJ_VAL(copy_string(vm, "machine", 7)),
                 OBJ_VAL(copy_string(vm, os.machine, strlen(os.machine))));

  RETURN_OBJ(dict);
}

DECLARE_MODULE_METHOD(os_sleep) {
  ENFORCE_ARG_COUNT(sleep, 1);
  ENFORCE_ARG_TYPE(sleep, 0, IS_NUMBER);
  sleep((int)AS_NUMBER(args[0]));
  RETURN;
}

static b_func_reg os_class_functions[] = {
    {"info", true, GET_MODULE_METHOD(os_info)},
    {"exec", true, GET_MODULE_METHOD(os_exec)},
    {"sleep", true, GET_MODULE_METHOD(os_sleep)},
    {NULL, false, NULL},
};

static b_class_reg klasses[] = {
    {"Os", os_class_functions},
    {NULL, NULL},
};

static b_module_reg module = {NULL, klasses};

CREATE_MODULE_LOADER(os) { return module; }