#ifndef bird_compat_asprintf_h
#define bird_compat_asprintf_h

#if defined(__GNUC__) && !defined(_GNU_SOURCE)
#define _GNU_SOURCE /* needed for (v)asprintf, affects '#include <stdio.h>' */
#endif

#include <stdarg.h> /* needed for va_*         */
#include <stdio.h>  /* needed for vsnprintf    */
#include <stdlib.h> /* needed for malloc, free */

/*
 * vscprintf:
 * MSVC implements this as _vscprintf, thus we just 'symlink' it here
 * GNU-C-compatible compilers do not implement this, thus we implement it here
 */
#ifdef _WIN32
#define vscprintf _vscprintf
#endif

#if !defined(__APPLE__) && !defined(__MACH__) && !defined(_WIN32)

int vscprintf(const char *format, va_list ap) {
  va_list ap_copy;
  va_copy(ap_copy, ap);
  int retval = vsnprintf(NULL, 0, format, ap_copy);
  va_end(ap_copy);
  return retval;
}

#endif

/*
 * asprintf, vasprintf:
 * MSVC does not implement these, thus we implement them here
 * GNU-C-compatible compilers implement these with the same names, thus we
 * don't have to do anything
 */
#if !defined(__APPLE__) && !defined(__MACH__)

int vasprintf(char **strp, const char *format, va_list ap) {
  int len = vscprintf(format, ap);
  if (len == -1)
    return -1;
  char *str = (char *)calloc(len + 1, sizeof(char));
  if (!str)
    return -1;
  int retval = vsnprintf(str, len + 1, format, ap);
  if (retval == -1) {
    free(str);
    return -1;
  }
  *strp = str;
  return retval;
}

int asprintf(char **strp, const char *format, ...) {
  va_list ap;
  va_start(ap, format);
  int retval = vasprintf(strp, format, ap);
  va_end(ap);
  return retval;
}
#endif

#endif // bird_compat_asprintf_h