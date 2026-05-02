#include "util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>

char *append_strings_n(char *old, char *new_str, size_t new_len) {
  // quick exit...
  if(new_str == NULL) {
    return old;
  }

  // find the size of the string to allocate
  const size_t old_len = NULL == old ? 0 : strlen(old);
  if (new_len > SIZE_MAX - old_len - 1) {
    return old;
  }

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
  if (new_str == NULL) {
    return old;
  }

  return append_strings_n(old, new_str, strlen(new_str));
}

char *read_file(const char *path) {
  FILE *fp = fopen(path, "rb");

  // file not readable (maybe due to permission)
  if (fp == NULL) {
    return NULL;
  }

  if (fseek(fp, 0L, SEEK_END) != 0) {
    fclose(fp);
    return NULL;
  }

  long file_size_long = ftell(fp);
  if (file_size_long < 0) {
    fclose(fp);
    return NULL;
  }

  size_t file_size = (size_t) file_size_long;

  if (file_size == SIZE_MAX) {
    fclose(fp);
    return NULL;
  }

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

// --------- START FILE SIZE MANIPULATION ---------------

static char *human_readable_filesize_suffix = "kKmMgGtT";
static const char     *human_readable_filesize_labels[]   = { "TiB", "GiB", "MiB", "KiB", "B" };

size_t *parse_size(char *input, size_t *target) {
  char *endp = input;
  char *match = NULL;
  size_t shift = 0;
  errno = 0;

  long double value = strtold(input, &endp);
  if(errno || endp == input || value < 0) {
    return NULL;
  }

  if(!(match = strchr(human_readable_filesize_suffix, *endp))) {
    return NULL;
  }

  if(*match) {
    shift = ((match - human_readable_filesize_suffix) / 2 + 1) * 10;
  }

  *target = value * (1LU << shift);

  return target;
}

char *format_size(size_t size) {

#define DIM(x) (sizeof(x)/sizeof(*(x)))
#define terabytes (1024UL * 1024UL * 1024UL * 1024UL)

  char    stack_buffer[64];
  char    *result = NULL;
  size_t  multiplier = terabytes;
  int i;

  for (i = 0; i < DIM(human_readable_filesize_labels); i++, multiplier /= 1024) {
    if (size < multiplier) {
      continue;
    }

    if (size % multiplier == 0) {
      snprintf(stack_buffer, sizeof(stack_buffer), "%lu %s",
               (unsigned long) (size / multiplier),
               human_readable_filesize_labels[i]);
    } else {
      snprintf(stack_buffer, sizeof(stack_buffer), "%.1f %s",
               (float) size / multiplier,
               human_readable_filesize_labels[i]);
    }

    result = (char *) malloc(strlen(stack_buffer) + 1);
    if (result == NULL) {
      return NULL;
    }

    strcpy(result, stack_buffer);
    return result;
  }

  result = (char *) malloc(2);
  if (result == NULL) {
    return NULL;
  }

  strcpy(result, "0");
  return result;

#undef terabytes
#undef DIM

}

// ----------- END FILE SIZE MANIPULATION ---------------