#ifndef BLADE_UTIL_H
#define BLADE_UTIL_H

#include "common.h"

char *append_strings(char *old, char *new_str);
char *append_strings_n(char *old, char *new_str, size_t new_len);
char *read_file(const char *path);

#endif