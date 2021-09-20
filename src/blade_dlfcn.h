#ifndef BLADE_BLADE_DLFCN_H
#define BLADE_BLADE_DLFCN_H

// Based on Stack Overflow answer at https://stackoverflow.com/a/53532799/5125586
// @TODO: Make implementation thread safe...

#define RTLD_GLOBAL 0x100 /* do not hide entries in this module */
#define RTLD_LOCAL  0x000 /* hide entries in this module */

#define RTLD_LAZY   0x000 /* accept unresolved externs */
#define RTLD_NOW    0x001 /* abort if module has unresolved externs */

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

static struct {
  long lasterror;
  const char *err_rutin;
} var = {
    0,
    NULL
};

void *dlopen(const char *filename, int flags) {
  HINSTANCE hInst;

  hInst= LoadLibrary (filename);
  if (hInst==NULL) {
    var.lasterror = GetLastError ();
    var.err_rutin = "dlopen";
  }
  return hInst;
}

int dlclose(void *handle) {
  BOOL ok;
  int rc= 0;

  ok= FreeLibrary ((HINSTANCE)handle);
  if (! ok) {
    var.lasterror = GetLastError ();
    var.err_rutin = "dlclose";
    rc= -1;
  }
  return rc;
}

void *dlsym(void *handle, const char *name){
  FARPROC fp;

  fp= GetProcAddress ((HINSTANCE)handle, name);
  if (!fp) {
    var.lasterror = GetLastError ();
    var.err_rutin = "dlsym";
  }
  return (void *)(intptr_t)fp;
}

const char *dlerror(void) {
  if (var.lasterror) {
    const char *m = "%s error #%ld";
    int length = snprintf (NULL, 0, m, var.err_rutin, var.lasterror);
    char *err = (char*) calloc(length + 1, sizeof(char));
    sprintf(err, m, var.err_rutin, var.lasterror);
    return err;
  } else {
    return NULL;
  }
}

#ifdef __cplusplus
}
#endif

#endif //BLADE_BLADE_DLFCN_H
