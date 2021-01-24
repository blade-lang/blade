#include <stdlib.h>

#include "bstring.h"

/**
 * string.length()
 *
 * returns the length of a string
 */
DECLARE_METHOD(stringlength) {
  ENFORCE_ARG_COUNT(string.length, 0);
  RETURN_NUMBER(AS_STRING(METHOD_OBJECT)->length);
}