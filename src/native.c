#include <stdlib.h>

#include "compat/unistd.h"
#include "native.h"
#include "time.h"
#include "vm.h"

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

/**
 * hasprop(object: instance, name: string)
 *
 * returns true if object has the property name or not
 */
DECLARE_NATIVE(hasprop) {
  ENFORCE_ARG_COUNT(hasprop, 2);
  ENFORCE_ARG_TYPE(hasprop, 0, IS_INSTANCE);
  ENFORCE_ARG_TYPE(hasprop, 1, IS_STRING);

  b_obj_instance *instance = AS_INSTANCE(args[0]);
  b_value dummy;
  RETURN_BOOL(table_get(&instance->fields, args[1], &dummy));
}

/**
 * getprop(object: instance, name: string)
 *
 * returns the property of object matching the name
 * or nil if the object contains no property with a matching
 * name
 */
DECLARE_NATIVE(getprop) {
  ENFORCE_ARG_COUNT(getprop, 2);
  ENFORCE_ARG_TYPE(getprop, 0, IS_INSTANCE);
  ENFORCE_ARG_TYPE(getprop, 1, IS_STRING);

  b_obj_instance *instance = AS_INSTANCE(args[0]);
  b_value value;
  table_get(&instance->fields, args[1], &value);
  return value;
}

/**
 * setprop(object: instance, name: string, value: any)
 *
 * sets the name property of object to value
 * if the property already exist, it overwrites it
 */
DECLARE_NATIVE(setprop) {
  ENFORCE_ARG_COUNT(setprop, 3);
  ENFORCE_ARG_TYPE(setprop, 0, IS_INSTANCE);
  ENFORCE_ARG_TYPE(setprop, 1, IS_STRING);

  b_obj_instance *instance = AS_INSTANCE(args[0]);
  b_value value;
  table_set(vm, &instance->fields, args[1], args[2]);
  RETURN;
}