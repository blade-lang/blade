#include <stdlib.h>

#include "compat/unistd.h"
#include "native.h"
#include "time.h"

/**
 * time()
 *
 * returns the current timestamp in seconds
 */
DECLARE_NATIVE(time) {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  RETURN_NUMBER((double)(1000000 * tv.tv_sec + tv.tv_usec) / 1000000);
}

/**
 * microtime()
 *
 * returns the current time in microseconds
 */
DECLARE_NATIVE(microtime) {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  RETURN_NUMBER(1000000 * tv.tv_sec + tv.tv_usec);
}

/**
 * id(value: any)
 *
 * returns the unique identifier of value within the system
 */
DECLARE_NATIVE(id) { RETURN_NUMBER((long)&args[0]); }