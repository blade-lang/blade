#ifndef BLADE_UTF8_H
#define BLADE_UTF8_H

#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "util.h"

#if defined(_MSC_VER)
#define b_nonnull
#define b_pure
#define b_restrict __restrict
#define b_weak __inline
#elif defined(__clang__) || defined(__GNUC__)
#define b_nonnull __attribute__((nonnull))
#define b_pure __attribute__((pure))
#define b_restrict __restrict__
#define b_weak __attribute__((weak))
#else
#error Non clang, non gcc, non MSVC compiler found!
#endif

bool utf8_islower(int c);
bool utf8_isupper(int c);
int utf8_get_lower(int c);
int utf8_get_upper(int c);
char *utf8_encode(unsigned int code);
int utf8_number_bytes(int value);
int utf8_decode(const uint8_t *bytes, uint32_t length);
int utf8length(char *s);
char *utf8index(char *s, int pos);
void utf8slice(char *s, int *start, int *end);
char *utf8_toupper(char *s, int length);
char *utf8_tolower(char *s, int length);
char *utf8_strstr(const char *haystack, const char *needle);

#endif //BLADE_UTF8_H
