#ifndef bird_util_h
#define bird_util_h

#include "common.h"

char *utf8_encode(unsigned int code);
int utf8_number_bytes(int value);
int utf8_decode_num_bytes(uint8_t byte);
int utf8_decode(const uint8_t *bytes, uint32_t length);
char *append_strings(const char *old, const char *new_str);

#endif