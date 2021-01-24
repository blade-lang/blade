#include <stdlib.h>

#include "list.h"

/**
 * list.length()
 *
 * returns the length of a list
 */
DECLARE_METHOD(listlength) {
  ENFORCE_ARG_COUNT(list.length, 0);
  RETURN_NUMBER(AS_LIST(METHOD_OBJECT)->items.count);
}