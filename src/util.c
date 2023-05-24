#include "util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *append_strings_n(char *old, char *new_str, size_t new_len) {
  // quick exit...
  if(new_str == NULL) {
    return old;
  }

  // find the size of the string to allocate
  const size_t old_len = strlen(old);
  const size_t out_len = old_len + new_len;

  // allocate a pointer to the new string
  char *out = (char *) realloc((void *) old, out_len + 1);

  // concat both strings and return
  if (out != NULL) {
    memcpy(out + old_len, new_str, new_len);
    out[out_len] = '\0';
    return out;
  }

  return old;
}

char *append_strings(char *old, char *new_str) {
  return append_strings_n(old, new_str, strlen(new_str));
}

char *read_file(const char *path) {
  FILE *fp = fopen(path, "rb");

  // file not readable (maybe due to permission)
  if (fp == NULL) {
    return NULL;
  }

  fseek(fp, 0L, SEEK_END);
  size_t file_size = ftell(fp);
  rewind(fp);

  char *buffer = (char *) malloc(file_size + 1);

  // the system might not have enough memory to read the file.
  if (buffer == NULL) {
    fclose(fp);
    return NULL;
  }

  size_t bytes_read = fread(buffer, sizeof(char), file_size, fp);

  // if we couldn't read the entire file
  if (bytes_read < file_size) {
    fclose(fp);
    free(buffer);
    return NULL;
  }

  buffer[bytes_read] = '\0';

  fclose(fp);
  return buffer;
}
