#ifndef BIRD_COMPAT_SPRINTF_H
#define BIRD_COMPAT_SPRINTF_H

#ifdef __GNUC__
// we want TR 24731-2:2010 extensions
#ifndef __STDC_WANT_LIB_EXT2__
#define __STDC_WANT_LIB_EXT2__ 1
#endif
#ifndef _GNU_SOURCE
#define _GNU_SOURCE /* needed for (v)asprintf, affects '#include <stdio.h>' */
#endif
#endif

#include <stdarg.h> /* needed for va_*         */
#include <stdio.h>  /* needed for vsnprintf    */
#include <stdlib.h> /* needed for malloc, free */

#ifdef _WIN32

int vasprintf(char **strp, const char *fmt, va_list ap) {
    // _vscprintf tells you how big the buffer needs to be
    int len = _vscprintf(fmt, ap);
    if (len == -1) {
        return -1;
    }
    size_t size = (size_t)len + 1;
    char *str = malloc(size);
    if (!str) {
        return -1;
    }
    // _vsprintf_s is the "secure" version of vsprintf
    int r = _vsprintf_s(str, len + 1, fmt, ap);
    if (r == -1) {
        free(str);
        return -1;
    }
    *strp = str;
    return r;
}

int asprintf(char **strp, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int r = vasprintf(strp, fmt, ap);
    va_end(ap);
    return r;
}

#endif

#endif // BIRD_COMPAT_SPRINTF_H