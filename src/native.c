#include "native.h"
#include "blade_time.h"
#include "vm.h"

#ifdef _WIN32
#include "win32.h"
#endif

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static b_obj_string *bin_to_string(b_vm *vm, int n) {
  // To store the binary number
  long long number = 0;
  int cnt = 0;
  while (n != 0) {
    int rem = n % 2;
    long long c = (long long) pow(10, cnt);
    number += rem * c;
    n /= 2;

    // Count used to store exponent value
    cnt++;
  }

  char str[66]; // assume maximum of 64 bits + 2 binary indicators (0b)
  int length = sprintf(str, "0b%lld", number);

  return copy_string(vm, str, length);
}

static b_obj_string *number_to_oct(b_vm *vm, long long n, bool numeric) {
  char str[66]; // assume maximum of 64 bits + 2 octal indicators (0c)
  int length = sprintf(str, numeric ? "0c%llo" : "%llo", n);

  return copy_string(vm, str, length);
}

static b_obj_string *number_to_hex(b_vm *vm, long long n, bool numeric) {
  char str[66]; // assume maximum of 64 bits + 2 hex indicators (0x)
  int length = sprintf(str, numeric ? "0x%llx" : "%llx", n);

  return copy_string(vm, str, length);
}

/**
 * time()
 *
 * returns the current timestamp in seconds
 */
DECLARE_NATIVE(time) {
  ENFORCE_ARG_COUNT(time, 0);

  struct timeval tv;
  gettimeofday(&tv, NULL);
#ifndef _WIN32
  RETURN_NUMBER((double) (1000000 * (double) tv.tv_sec + (double) tv.tv_usec) /
                1000000);
#else
  RETURN_NUMBER((double)tv.tv_sec + ((double)tv.tv_usec / 10000000));
#endif // !_WIN32
}

/**
 * microtime()
 *
 * returns the current time in microseconds
 */
DECLARE_NATIVE(microtime) {
  ENFORCE_ARG_COUNT(microtime, 0);

  struct timeval tv;
  gettimeofday(&tv, NULL);
#ifndef _WIN32
  RETURN_NUMBER(1000000 * tv.tv_sec + tv.tv_usec);
#else
  RETURN_NUMBER((1000000 * (double)tv.tv_sec) + ((double)tv.tv_usec / 10));
#endif // !_WIN32
}

/**
 * id(value: any)
 *
 * returns the unique identifier of value within the system
 */
DECLARE_NATIVE(id) {
  ENFORCE_ARG_COUNT(id, 1);

#ifdef _WIN32
  RETURN_NUMBER(PtrToLong(&args[0]));
#else
  RETURN_NUMBER((long) &args[0]);
#endif
}

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
  RETURN_BOOL(table_get(&instance->properties, args[1], &dummy));
}

/**
 * getprop(object: instance, name: string)
 *
 * returns the property of the object matching the given name
 * or nil if the object contains no property with a matching
 * name
 */
DECLARE_NATIVE(getprop) {
  ENFORCE_ARG_COUNT(getprop, 2);
  ENFORCE_ARG_TYPE(getprop, 0, IS_INSTANCE);
  ENFORCE_ARG_TYPE(getprop, 1, IS_STRING);

  b_obj_instance *instance = AS_INSTANCE(args[0]);
  b_value value;
  table_get(&instance->properties, args[1], &value);
  RETURN_VALUE(value);
}

/**
 * setprop(object: instance, name: string, value: any)
 *
 * sets the named property of the object to value.
 *
 * if the property already exist, it overwrites it
 * @returns bool: true if a new property was set, false if a property was
 * updated
 */
DECLARE_NATIVE(setprop) {
  ENFORCE_ARG_COUNT(setprop, 3);
  ENFORCE_ARG_TYPE(setprop, 0, IS_INSTANCE);
  ENFORCE_ARG_TYPE(setprop, 1, IS_STRING);

  b_obj_instance *instance = AS_INSTANCE(args[0]);
  RETURN_BOOL(table_set(vm, &instance->properties, args[1], args[2]));
}

/**
 * delprop(object: instance, name: string)
 *
 * deletes the named property from the object
 * @returns bool
 */
DECLARE_NATIVE(delprop) {
  ENFORCE_ARG_COUNT(delprop, 2);
  ENFORCE_ARG_TYPE(delprop, 0, IS_INSTANCE);
  ENFORCE_ARG_TYPE(delprop, 1, IS_STRING);

  b_obj_instance *instance = AS_INSTANCE(args[0]);
  RETURN_BOOL(table_delete(&instance->properties, args[1]));
}

/**
 * max(number...)
 *
 * returns the greatest of the number arguments
 */
DECLARE_NATIVE(max) {
  ENFORCE_MIN_ARG(max, 2);
  ENFORCE_ARG_TYPE(max, 0, IS_NUMBER);

  double max = AS_NUMBER(args[0]);

  for (int i = 1; i < arg_count; i++) {
    ENFORCE_ARG_TYPE(max, i, IS_NUMBER);
    double number = AS_NUMBER(args[i]);
    if (number > max)
      max = number;
  }

  RETURN_NUMBER(max);
}

/**
 * min(number...)
 *
 * returns the least of the number arguments
 */
DECLARE_NATIVE(min) {
  ENFORCE_MIN_ARG(min, 2);
  ENFORCE_ARG_TYPE(min, 0, IS_NUMBER);

  double min = AS_NUMBER(args[0]);

  for (int i = 1; i < arg_count; i++) {
    ENFORCE_ARG_TYPE(min, i, IS_NUMBER);
    double number = AS_NUMBER(args[i]);
    if (number < min)
      min = number;
  }

  RETURN_NUMBER(min);
}

/**
 * sum(number...)
 *
 * returns the summation of all numbers given
 */
DECLARE_NATIVE(sum) {
  ENFORCE_MIN_ARG(sum, 2);

  double sum = 0;
  for (int i = 0; i < arg_count; i++) {
    ENFORCE_ARG_TYPE(sum, i, IS_NUMBER);
    sum += AS_NUMBER(args[i]);
  }

  RETURN_NUMBER(sum);
}

/**
 * abs(x: number)
 *
 * returns the absolute value of a number.
 *
 * if x is not a number but it's class defines a method @to_abs(),
 * returns the result of calling x.to_abs()
 */
DECLARE_NATIVE(abs) {
  ENFORCE_ARG_COUNT(abs, 1);

  // handle classes that define a to_abs() method.
  METHOD_OVERRIDE(to_abs, 6);

  ENFORCE_ARG_TYPE(abs, 0, IS_NUMBER);
  double value = AS_NUMBER(args[0]);

  if (value > -1) RETURN_VALUE(args[0]);
  RETURN_NUMBER(-value);
}

/**
 * int(i: number)
 *
 * returns the integer of a number or 0 if no number is given.
 *
 * if i is not a number but it's class defines @to_number(), it
 * returns the result of calling to_number()
 */
DECLARE_NATIVE(int) {
  ENFORCE_ARG_RANGE(int, 0, 1);

  if (arg_count == 0) {
    RETURN_NUMBER(0);
  }

  // handle classes that define a to_number() method.
  METHOD_OVERRIDE(to_number, 9);

  ENFORCE_ARG_TYPE(int, 0, IS_NUMBER);
  RETURN_NUMBER((double) ((int) AS_NUMBER(args[0])));
}

/**
 * bin(x: number)
 *
 * converts a number to it's binary string.
 *
 * if i is not a number but it's class defines @to_bin(), it
 * returns the result of calling bin(x.to_bin())
 */
DECLARE_NATIVE(bin) {
  ENFORCE_ARG_COUNT(bin, 1);

  // handle classes that define a to_bin() method.
  METHOD_OVERRIDE(to_bin, 6);

  ENFORCE_ARG_TYPE(bin, 0, IS_NUMBER);
  RETURN_OBJ(bin_to_string(vm, AS_NUMBER(args[0])));
}

/**
 * oct(x: number)
 *
 * converts a number to it's octal string.
 *
 * if i is not a number but it's class defines @to_oct(), it
 * returns the result of calling oct(x.to_oct())
 */
DECLARE_NATIVE(oct) {
  ENFORCE_ARG_COUNT(oct, 1);

  // handle classes that define a to_oct() method.
  METHOD_OVERRIDE(to_oct, 6);

  ENFORCE_ARG_TYPE(oct, 0, IS_NUMBER);
  RETURN_OBJ(number_to_oct(vm, AS_NUMBER(args[0]), false));
}

/**
 * hex(x: number)
 *
 * converts a number to it's hexadecimal string.
 *
 * if i is not a number but it's class defines @to_hex(), it
 * returns the result of calling hex(x.to_hex())
 */
DECLARE_NATIVE(hex) {
  ENFORCE_ARG_COUNT(hex, 1);

  // handle classes that define a to_hex() method.
  METHOD_OVERRIDE(to_hex, 6);

  ENFORCE_ARG_TYPE(hex, 0, IS_NUMBER);
  RETURN_OBJ(number_to_hex(vm, AS_NUMBER(args[0]), false));
}

/**
 * to_bool(value: any)
 *
 * converts a value into a boolean.
 *
 * classes may override the return value by declaring a @to_bool()
 * function.
 */
DECLARE_NATIVE(to_bool) {
  ENFORCE_ARG_COUNT(to_bool, 1);
  METHOD_OVERRIDE(to_bool, 7);
  RETURN_BOOL(!is_false(args[0]));
}

/**
 * to_string(value: any)
 *
 * convert a value into a string.
 *
 * native classes may override the return value by declaring a @to_string()
 * function.
 */
DECLARE_NATIVE(to_string) {
  ENFORCE_ARG_COUNT(to_string, 1);
  METHOD_OVERRIDE(to_string, 9);
  char *result = value_to_string(vm, args[0]);
  RETURN_STRING(result);
}

/**
 * to_number(value: any)
 *
 * convert a value into a number.
 *
 * native classes may override the return value by declaring a @to_number()
 * function.
 */
DECLARE_NATIVE(to_number) {
  ENFORCE_ARG_COUNT(to_number, 1);
  METHOD_OVERRIDE(to_number, 9);

  if (IS_NUMBER(args[0])) {
    RETURN_VALUE(args[0]);
  } else if (IS_BOOL(args[0])) {
    RETURN_NUMBER(AS_BOOL(args[0]) ? 1 : 0);
  } else if (IS_NIL(args[0])) {
    RETURN_NUMBER(-1);
  }
  RETURN_NUMBER(strtod((const char *) value_to_string(vm, args[0]), NULL));
}

/**
 * to_int(value: any)
 *
 * convert a value into an integer.
 *
 * native classes may override the return value by declaring a @to_int()
 * function.
 */
DECLARE_NATIVE(to_int) {
  ENFORCE_ARG_COUNT(to_int, 1);
  METHOD_OVERRIDE(to_int, 6);
  ENFORCE_ARG_TYPE(to_int, 0, IS_NUMBER);
  RETURN_NUMBER((int) AS_NUMBER(args[0]));
}

/**
 * to_list(value: any)
 *
 * convert a value into a list.
 *
 * native classes may override the return value by declaring a @to_list()
 * function.
 */
DECLARE_NATIVE(to_list) {
  ENFORCE_ARG_COUNT(to_list, 1);
  METHOD_OVERRIDE(to_list, 0);

  if (IS_LIST(args[0])) {
    RETURN_VALUE(args[0]);
  }

  b_obj_list *list = (b_obj_list *) GC(new_list(vm));

  if (IS_DICT(args[0])) {
    b_obj_dict *dict = AS_DICT(args[0]);
    for (int i = 0; i < dict->names.count; i++) {
      b_obj_list *n_list = (b_obj_list *) GC(new_list(vm));
      write_value_arr(vm, &n_list->items, dict->names.values[i]);

      b_value value;
      table_get(&dict->items, dict->names.values[i], &value);
      write_value_arr(vm, &n_list->items, value);

      write_value_arr(vm, &list->items, OBJ_VAL(n_list));
    }
  } else if(IS_STRING(args[0])) {
    b_obj_string *str = AS_STRING(args[0]);
    for(int i = 0; i < str->utf8_length; i++) {
      int start = i, end = i + 1;
      utf8slice(str->chars, &start, &end);

      write_list(vm, list, STRING_L_VAL(str->chars + start, (int) (end - start)));
    }
  } else if(IS_RANGE(args[0])) {
    b_obj_range *range = AS_RANGE(args[0]);
    if(range->upper > range->lower) {
      for(int i = range->lower; i < range->upper; i++) {
        write_list(vm, list, NUMBER_VAL(i));
      }
    } else {
      for(int i = range->lower; i > range->upper; i--) {
        write_list(vm, list, NUMBER_VAL(i));
      }
    }
  } else {
    write_value_arr(vm, &list->items, args[0]);
  }

  RETURN_OBJ(list);
}

/**
 * to_dict(value: any)
 *
 * convert a value into a dictionary.
 *
 * native classes may override the return value by declaring a @to_dict()
 * function.
 */
DECLARE_NATIVE(to_dict) {
  ENFORCE_ARG_COUNT(to_dict, 1);
  METHOD_OVERRIDE(to_dict, 7);

  if (IS_DICT(args[0])) {
    RETURN_VALUE(args[0]);
  }

  b_obj_dict *dict = (b_obj_dict *) GC(new_dict(vm));
  dict_set_entry(vm, dict, NUMBER_VAL(0), args[0]);

  RETURN_OBJ(dict);
}

/**
 * chr(i: number)
 *
 * return the string representing a character whose Unicode
 * code point is the number i.
 */
DECLARE_NATIVE(chr) {
  ENFORCE_ARG_COUNT(chr, 1);
  ENFORCE_ARG_TYPE(chr, 0, IS_NUMBER);
  char *string = utf8_encode((int) AS_NUMBER(args[0]));
  RETURN_STRING(string);
}

/**
 * ord(ch: char)
 *
 * return the code point value of a unicode character.
 */
DECLARE_NATIVE(ord) {
  ENFORCE_ARG_COUNT(ord, 1);
  ENFORCE_ARG_TYPE(ord, 0, IS_STRING);
  b_obj_string *string = AS_STRING(args[0]);

  int max_length = string->length > 1 && (int) string->chars[0] < 1 ? 3 : 1;

  if (string->length > max_length) {
    RETURN_ERROR("ord() expects character as argument, string given");
  }

  const uint8_t *bytes = (uint8_t *) string->chars;
  if ((bytes[0] & 0xc0) == 0x80) {
    RETURN_NUMBER(-1);
  }

  // Decode the UTF-8 sequence.
  RETURN_NUMBER(utf8_decode((uint8_t *) string->chars, string->length));
}

/**
 * rand([limit: number, [upper: number]])
 *
 * - returns a random number between 0 and 1 if no argument is given
 * - returns a random number between 0 and limit if one argument is given
 * - returns a random number between limit and upper if two arguments is
 * given
 */
DECLARE_NATIVE(rand) {
  ENFORCE_ARG_RANGE(rand, 0, 2);
  int lower_limit = 0;
  int upper_limit = 1;

  if (arg_count > 0) {
    ENFORCE_ARG_TYPE(rand, 0, IS_NUMBER);
    lower_limit = AS_NUMBER(args[0]);
  }
  if (arg_count == 2) {
    ENFORCE_ARG_TYPE(rand, 1, IS_NUMBER);
    upper_limit = AS_NUMBER(args[1]);
  }

  if (lower_limit > upper_limit) {
    int tmp = upper_limit;
    upper_limit = lower_limit;
    lower_limit = tmp;
  }

  int n = upper_limit - lower_limit + 1;
  int remainder = RAND_MAX % n;
  int x;

  struct timeval tv;
  gettimeofday(&tv, NULL);
  srand((unsigned int) (1000000 * tv.tv_sec + tv.tv_usec));
  do {
    x = rand();
  } while (x >= RAND_MAX - remainder);

  RETURN_NUMBER((double) lower_limit + x % n);
}

/**
 * type(value: any)
 *
 * returns the name of the type of value
 */
DECLARE_NATIVE(typeof) {
  ENFORCE_ARG_COUNT(typeof, 1);
  char *result = (char *) value_type(args[0]);
  RETURN_STRING(result);
}

/**
 * is_callable(value: any)
 *
 * returns true if the value is a callable function or class and false otherwise
 */
DECLARE_NATIVE(is_callable) {
  ENFORCE_ARG_COUNT(is_callable, 1);
  RETURN_BOOL(IS_CLASS(args[0]) || IS_FUNCTION(args[0]) ||
              IS_CLOSURE(args[0]) || IS_BOUND(args[0]) || IS_NATIVE(args[0]));
}

/**
 * is_bool(value: any)
 *
 * returns true if the value is a boolean or false otherwise
 */
DECLARE_NATIVE(is_bool) {
  ENFORCE_ARG_COUNT(is_bool, 1);
  RETURN_BOOL(IS_BOOL(args[0]));
}

/**
 * is_number(value: any)
 *
 * returns true if the value is a number or false otherwise
 */
DECLARE_NATIVE(is_number) {
  ENFORCE_ARG_COUNT(is_number, 1);
  RETURN_BOOL(IS_NUMBER(args[0]));
}

/**
 * is_int(value: any)
 *
 * returns true if the value is an integer or false otherwise
 */
DECLARE_NATIVE(is_int) {
  ENFORCE_ARG_COUNT(is_int, 1);
  RETURN_BOOL(IS_NUMBER(args[0]) &&
              (((int) AS_NUMBER(args[0])) == AS_NUMBER(args[0])));
}

/**
 * is_string(value: any)
 *
 * returns true if the value is a string or false otherwise
 */
DECLARE_NATIVE(is_string) {
  ENFORCE_ARG_COUNT(is_string, 1);
  RETURN_BOOL(IS_STRING(args[0]));
}

/**
 * is_bytes(value: any)
 *
 * returns true if the value is a bytes or false otherwise
 */
DECLARE_NATIVE(is_bytes) {
  ENFORCE_ARG_COUNT(is_bytes, 1);
  RETURN_BOOL(IS_BYTES(args[0]));
}

/**
 * is_list(value: any)
 *
 * returns true if the value is a list or false otherwise
 */
DECLARE_NATIVE(is_list) {
  ENFORCE_ARG_COUNT(is_list, 1);
  RETURN_BOOL(IS_LIST(args[0]));
}

/**
 * is_dict(value: any)
 *
 * returns true if the value is a dictionary or false otherwise
 */
DECLARE_NATIVE(is_dict) {
  ENFORCE_ARG_COUNT(is_dict, 1);
  RETURN_BOOL(IS_DICT(args[0]));
}

/**
 * is_object(value: any)
 *
 * returns true if the value is an object or false otherwise
 */
DECLARE_NATIVE(is_object) {
  ENFORCE_ARG_COUNT(is_object, 1);
  RETURN_BOOL(IS_OBJ(args[0]));
}

/**
 * is_function(value: any)
 *
 * returns true if the value is a function or false otherwise
 */
DECLARE_NATIVE(is_function) {
  ENFORCE_ARG_COUNT(is_function, 1);
  RETURN_BOOL(IS_FUNCTION(args[0]) || IS_CLOSURE(args[0]) ||
              IS_BOUND(args[0]) || IS_NATIVE(args[0]));
}

/**
 * is_iterable(value: any)
 *
 * returns true if the value is an iterable or false otherwise
 */
DECLARE_NATIVE(is_iterable) {
  ENFORCE_ARG_COUNT(is_iterable, 1);
  bool is_iterable = IS_LIST(args[0]) || IS_DICT(args[0]) || IS_STRING(args[0]) || IS_BYTES(args[0]);
  if(!is_iterable && IS_INSTANCE(args[0])) {
      b_obj_class *klass = AS_INSTANCE(args[0])->klass;
      b_value dummy;
      is_iterable = table_get(&klass->methods, STRING_VAL("@iter"), &dummy)
              && table_get(&klass->methods, STRING_VAL("@itern"), &dummy);
  }
  RETURN_BOOL( is_iterable);
}

/**
 * is_class(value: any)
 *
 * returns true if the value is a class or false otherwise
 */
DECLARE_NATIVE(is_class) {
  ENFORCE_ARG_COUNT(is_class, 1);
  RETURN_BOOL(IS_CLASS(args[0]));
}

/**
 * is_file(value: any)
 *
 * returns true if the value is a file or false otherwise
 */
DECLARE_NATIVE(is_file) {
  ENFORCE_ARG_COUNT(is_file, 1);
  RETURN_BOOL(IS_FILE(args[0]));
}

/**
 * is_instance(value: any, name: class)
 *
 * returns true if the value is an instance the given class, false
 * otherwise
 */
DECLARE_NATIVE(is_instance) {
  ENFORCE_ARG_COUNT(is_instance, 2);
  ENFORCE_ARG_TYPE(is_instance, 0, IS_INSTANCE);
  ENFORCE_ARG_TYPE(is_instance, 1, IS_CLASS);

  RETURN_BOOL(is_instance_of(AS_INSTANCE(args[0])->klass,
                             AS_CLASS(args[1])->name->chars));
}

//------------------------------------------------------------------------------

/**
 * print(...)
 *
 * prints values to the standard output
 */
DECLARE_NATIVE(print) {
  for (int i = 0; i < arg_count; i++) {
    print_value(args[i]);
    if (i != arg_count - 1) {
      printf(" ");
    }
  }
//  printf("\n");
  RETURN;
}

/**
 * println(...)
 *
 * Prints to stdout with a newline
 */
DECLARE_NATIVE(println) {
  for (int i = 0; i < arg_count; i++) {
    print_value(args[i]);
    if (i != arg_count - 1) {
      printf(" ");
    }
  }
  printf("\n");
  RETURN;
}
