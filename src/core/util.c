#include "util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// returns the number of bytes contained in a unicode character
int utf8_number_bytes(int value) {
  if (value < 0) {
    return -1;
  }

  if (value <= 0x7f)
    return 1;
  if (value <= 0x7ff)
    return 2;
  if (value <= 0xffff)
    return 3;
  if (value <= 0x10ffff)
    return 4;
  return 0;
}

char *utf8_encode(unsigned int code) {
  int count = utf8_number_bytes(code);
  if(count > 0 ){
    char * chars = (char*)calloc(count + 1, sizeof(char));
    if (code <= 0x7F) {
      chars[0] = (code & 0x7F);
      chars[1] = '\0';
    } else if (code <= 0x7FF) {
      // one continuation byte
      chars[1] = 0x80 | (code & 0x3F);
      code = (code >> 6);
      chars[0] = 0xC0 | (code & 0x1F);
    } else if (code <= 0xFFFF) {
      // two continuation bytes
      chars[2] = 0x80 | (code & 0x3F);
      code = (code >> 6);
      chars[1] = 0x80 | (code & 0x3F);
      code = (code >> 6);
      chars[0] = 0xE0 | (code & 0xF);
    } else if (code <= 0x10FFFF) {
      // three continuation bytes
      chars[3] = 0x80 | (code & 0x3F);
      code = (code >> 6);
      chars[2] = 0x80 | (code & 0x3F);
      code = (code >> 6);
      chars[1] = 0x80 | (code & 0x3F);
      code = (code >> 6);
      chars[0] = 0xF0 | (code & 0x7);
    } else {
      // unicode replacement character
      chars[2] = 0xEF;
      chars[1] = 0xBF;
      chars[0] = 0xBD;
    }
    return chars;
  }
  char result = (char)code;
  return &result;
}

int utf8_decode_num_bytes(uint8_t byte) {
  // If the byte starts with 10xxxxx, it's the middle of a UTF-8 sequence, so
  // don't count it at all.
  if ((byte & 0xc0) == 0x80)
    return 0;

  // The first byte's high bits tell us how many bytes are in the UTF-8
  // sequence.
  if ((byte & 0xf8) == 0xf0)
    return 4;
  if ((byte & 0xf0) == 0xe0)
    return 3;
  if ((byte & 0xe0) == 0xc0)
    return 2;
  return 1;
}

int utf8_decode(const uint8_t *bytes, uint32_t length) {

  // Single byte (i.e. fits in ASCII).
  if (*bytes <= 0x7f)
    return *bytes;

  int value;
  uint32_t remaining_bytes;
  if ((*bytes & 0xe0) == 0xc0) {
    // Two byte sequence: 110xxxxx 10xxxxxx.
    value = *bytes & 0x1f;
    remaining_bytes = 1;
  } else if ((*bytes & 0xf0) == 0xe0) {
    // Three byte sequence: 1110xxxx	 10xxxxxx 10xxxxxx.
    value = *bytes & 0x0f;
    remaining_bytes = 2;
  } else if ((*bytes & 0xf8) == 0xf0) {
    // Four byte sequence: 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx.
    value = *bytes & 0x07;
    remaining_bytes = 3;
  } else {
    // Invalid UTF-8 sequence.
    return -1;
  }

  // Don't read past the end of the buffer on truncated UTF-8.
  if (remaining_bytes > length - 1)
    return -1;

  while (remaining_bytes > 0) {
    bytes++;
    remaining_bytes--;

    // Remaining bytes must be of form 10xxxxxx.
    if ((*bytes & 0xc0) != 0x80)
      return -1;

    value = value << 6 | (*bytes & 0x3f);
  }

  return value;
}

char *append_strings(const char *old, const char *new_str) {
  // find the size of the string to allocate
  const size_t old_len = strlen(old), new_len = strlen(new_str);
  const size_t out_len = old_len + new_len + 1;

  // allocate a pointer to the new string
  char *out = malloc(out_len);

  // concat both strings and return
  memcpy(out, old, old_len);
  memcpy(out + old_len, new_str, new_len + 1);

  return out;
}

int read_line(char line[], int max) {
  int nch = 0;
  int c;
  max = max - 1; // leave room for '\0'

  while ((c = getchar()) != EOF) {
    if (c == '\n')
      break;

    if (nch < max) {
      line[nch] = *utf8_encode(c);
      nch = nch + 1;
    }
  }

  if (c == EOF && nch == 0)
    return EOF;

  line[nch] = '\0';

  return nch;
}

int utf8len(char *s) {
  int len = 0;
  for (; *s; ++s)
    if ((*s & 0xC0) != 0x80)
      ++len;
  return len;
}

// returns a pointer to the beginning of the pos'th utf8 codepoint
// in the buffer at s
char *utf8index(char *s, int pos) {
  ++pos;
  for (; *s; ++s) {
    if ((*s & 0xC0) != 0x80)
      --pos;
    if (pos == 0)
      return s;
  }
  return NULL;
}

// converts codepoint indexes start and end to byte offsets in the buffer at s
void utf8slice(char *s, int *start, int *end) {
  char *p = utf8index(s, *start);
  *start = p != NULL ? p - s : -1;
  p = utf8index(s, *end);
  *end = p != NULL ? p - s : (int)strlen(s);
}