#include "builtin/dict.h"

#include <stdlib.h>

/**
 * dictionary.length()
 *
 * returns the length of a dictionary
 */
DECLARE_METHOD(dictlength) {
  ENFORCE_ARG_COUNT(dictionary.length, 0);
  RETURN_NUMBER(AS_DICT(METHOD_OBJECT)->names.count);
}