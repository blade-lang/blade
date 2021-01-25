#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "bstring.h"
#include "memory.h"

DECLARE_STRING_METHOD(length) {
  ENFORCE_ARG_COUNT(string.length, 0);
  RETURN_NUMBER(AS_STRING(METHOD_OBJECT)->length);
}

DECLARE_STRING_METHOD(upper) {
  ENFORCE_ARG_COUNT(string.upper, 0);
  char *string = (char *)AS_CSTRING(METHOD_OBJECT);
  for (char *p = string; *p; p++)
    *p = toupper(*p);
  RETURN_TSTRING(string, AS_STRING(METHOD_OBJECT)->length);
}

DECLARE_STRING_METHOD(lower) {
  ENFORCE_ARG_COUNT(string.lower, 0);
  char *string = (char *)AS_CSTRING(METHOD_OBJECT);
  for (char *p = string; *p; p++)
    *p = tolower(*p);
  RETURN_TSTRING(string, AS_STRING(METHOD_OBJECT)->length);
}

DECLARE_STRING_METHOD(is_alpha) {
  ENFORCE_ARG_COUNT(string.is_alpha, 0);
  b_obj_string *string = AS_STRING(METHOD_OBJECT);
  for (int i = 0; i < string->length; i++) {
    if (!isalpha(string->chars[i]))
      RETURN_FALSE;
  }
  RETURN_TRUE;
}

DECLARE_STRING_METHOD(is_alnum) {
  ENFORCE_ARG_COUNT(string.is_alnum, 0);
  b_obj_string *string = AS_STRING(METHOD_OBJECT);
  for (int i = 0; i < string->length; i++) {
    if (!isalnum(string->chars[i]))
      RETURN_FALSE;
  }
  RETURN_TRUE;
}

DECLARE_STRING_METHOD(is_number) {
  ENFORCE_ARG_COUNT(string.is_number, 0);
  b_obj_string *string = AS_STRING(METHOD_OBJECT);
  for (int i = 0; i < string->length; i++) {
    if (!isnumber(string->chars[i]))
      RETURN_FALSE;
  }
  RETURN_TRUE;
}

DECLARE_STRING_METHOD(is_lower) {
  ENFORCE_ARG_COUNT(string.is_lower, 0);
  b_obj_string *string = AS_STRING(METHOD_OBJECT);
  for (int i = 0; i < string->length; i++) {
    if (!islower(string->chars[i]))
      RETURN_FALSE;
  }
  RETURN_TRUE;
}

DECLARE_STRING_METHOD(is_upper) {
  ENFORCE_ARG_COUNT(string.is_upper, 0);
  b_obj_string *string = AS_STRING(METHOD_OBJECT);
  for (int i = 0; i < string->length; i++) {
    if (!isupper(string->chars[i]))
      RETURN_FALSE;
  }
  RETURN_TRUE;
}

DECLARE_STRING_METHOD(is_space) {
  ENFORCE_ARG_COUNT(string.is_space, 0);
  b_obj_string *string = AS_STRING(METHOD_OBJECT);
  for (int i = 0; i < string->length; i++) {
    if (!isspace(string->chars[i]))
      RETURN_FALSE;
  }
  RETURN_TRUE;
}

DECLARE_STRING_METHOD(trim) {
  ENFORCE_ARG_RANGE(string.trim, 0, 1);

  char trimmer = '\0';

  if (arg_count == 1) {
    ENFORCE_ARG_TYPE(string.trim, 0, IS_CHAR);
    trimmer = (char)AS_STRING(args[0])->chars[0];
  }

  char *string = AS_CSTRING(METHOD_OBJECT);

  char *end;

  // Trim leading space
  if (trimmer == '\0') {
    while (isspace((unsigned char)*string))
      string++;
  } else {
    while (trimmer == *string)
      string++;
  }

  if (*string == 0) // All spaces?
    RETURN_OBJ(copy_string(vm, "", 0));

  // Trim trailing space
  end = string + strlen(string) - 1;
  if (trimmer == '\0') {
    while (end > string && isspace((unsigned char)*end))
      end--;
  } else {
    while (end > string && trimmer == *end)
      end--;
  }

  // Write new null terminator character
  end[1] = '\0';

  RETURN_STRING(string);
}

DECLARE_STRING_METHOD(ltrim) {
  ENFORCE_ARG_RANGE(string.ltrim, 0, 1);

  char trimmer = '\0';

  if (arg_count == 1) {
    ENFORCE_ARG_TYPE(string.ltrim, 0, IS_CHAR);
    trimmer = (char)AS_STRING(args[0])->chars[0];
  }

  char *string = AS_CSTRING(METHOD_OBJECT);

  char *end;

  // Trim leading space
  if (trimmer == '\0') {
    while (isspace((unsigned char)*string))
      string++;
  } else {
    while (trimmer == *string)
      string++;
  }

  if (*string == 0) // All spaces?
    RETURN_OBJ(copy_string(vm, "", 0));

  end = string + strlen(string) - 1;

  // Write new null terminator character
  end[1] = '\0';

  RETURN_STRING(string);
}

DECLARE_STRING_METHOD(rtrim) {
  ENFORCE_ARG_RANGE(string.rtrim, 0, 1);

  char trimmer = '\0';

  if (arg_count == 1) {
    ENFORCE_ARG_TYPE(string.rtrim, 0, IS_CHAR);
    trimmer = (char)AS_STRING(args[0])->chars[0];
  }

  char *string = AS_CSTRING(METHOD_OBJECT);

  char *end;

  if (*string == 0) // All spaces?
    RETURN_OBJ(copy_string(vm, "", 0));

  end = string + strlen(string) - 1;
  if (trimmer == '\0') {
    while (end > string && isspace((unsigned char)*end))
      end--;
  } else {
    while (end > string && trimmer == *end)
      end--;
  }

  // Write new null terminator character
  end[1] = '\0';

  RETURN_STRING(string);
}

DECLARE_STRING_METHOD(join) {
  ENFORCE_ARG_COUNT(string.join, 1);
  ENFORCE_ARG_TYPE(string.join, 0, IS_OBJ);

  b_value argument = args[0];
  int length = 0;
  char **array = NULL;

  if (IS_STRING(argument)) {
    // empty argument
    if (AS_STRING(argument)->length == 0)
      RETURN_VALUE(argument);

    char *string = (char *)AS_CSTRING(argument);
    length = AS_STRING(argument)->length;
    array = (char **)calloc(length, sizeof(char **));

    for (int i = 0; i < length; i++) {
      array[i] = (char *)calloc(2, sizeof(char *));
      char str[2] = {string[i], '\0'};
      strcpy(array[i], str);
    }
  } else if (IS_LIST(argument) || IS_DICT(argument)) {

    length = IS_LIST(argument) ? AS_LIST(argument)->items.count
                               : AS_DICT(argument)->names.count;
    b_value *values = IS_LIST(argument) ? AS_LIST(argument)->items.values
                                        : AS_DICT(argument)->names.values;

    if (length == 0)
      RETURN_VALUE(argument);

    array = (char **)calloc(length, sizeof(char **));

    for (int i = 0; i < length; i++) {
      // get interpreted string here
      array[i] = value_to_string(vm, values[i]);
    }
  } else {
    RETURN_ERROR("string.join() does not support object of type %s",
                 value_type(argument))
  }

  for (int i = 0; i < length; i++) {
    if (i != 0)
      strncat(array[0], array[i], strlen(array[i]));

    if (i != length - 1)
      strncat(array[0], AS_CSTRING(METHOD_OBJECT),
              strlen(AS_CSTRING(METHOD_OBJECT)));
  }

  RETURN_STRING(array[0]);
}

DECLARE_STRING_METHOD(split) {
  ENFORCE_ARG_COUNT(string.split, 1);
  ENFORCE_ARG_TYPE(string.split, 0, IS_STRING);

  b_obj_list *list = new_list(vm);

  if (AS_STRING(METHOD_OBJECT)->length == 0)
    return OBJ_VAL(list);

  // main work here...
  if (AS_STRING(args[0])->length > 0) {
    char *token, *str, *tofree;

    tofree = str =
        strdup(AS_CSTRING(METHOD_OBJECT)); // We own str's memory now.
    while ((token = strsep(&str, AS_CSTRING(args[0]))))
      write_value_arr(vm, &list->items,
                      OBJ_VAL(copy_string(vm, token, strlen(token))));
    free(tofree);
  } else {
    const char *string = AS_CSTRING(METHOD_OBJECT);
    for (int i = 0; i < (int)strlen(string); i++) {
      write_value_arr(vm, &list->items,
                      OBJ_VAL(copy_string(vm, &string[i], 1)));
    }
  }

  RETURN_OBJ(list);
}

DECLARE_STRING_METHOD(index_of) {
  ENFORCE_ARG_COUNT(string.index_of, 1);
  ENFORCE_ARG_TYPE(string.index_of, 0, IS_STRING);

  char *str = AS_CSTRING(METHOD_OBJECT);
  char *result = strstr(str, AS_CSTRING(args[0]));

  RETURN_NUMBER(result - str);
}

DECLARE_STRING_METHOD(starts_with) {
  ENFORCE_ARG_COUNT(string.starts_with, 1);
  ENFORCE_ARG_TYPE(string.starts_with, 0, IS_STRING);

  b_obj_string *string = AS_STRING(METHOD_OBJECT);
  b_obj_string *substr = AS_STRING(args[0]);

  if (string->length == 0 || substr->length == 0 ||
      substr->length > string->length)
    RETURN_FALSE;

  RETURN_BOOL(memcmp(substr->chars, string->chars, substr->length) == 0);
}

DECLARE_STRING_METHOD(ends_with) {
  ENFORCE_ARG_COUNT(string.ends_with, 1);
  ENFORCE_ARG_TYPE(string.ends_with, 0, IS_STRING);

  b_obj_string *string = AS_STRING(METHOD_OBJECT);
  b_obj_string *substr = AS_STRING(args[0]);

  if (string->length == 0 || substr->length == 0 ||
      substr->length > string->length)
    RETURN_FALSE;

  int difference = string->length - substr->length;

  RETURN_BOOL(
      memcmp(substr->chars, string->chars + difference, substr->length) == 0);
}

DECLARE_STRING_METHOD(count) {
  ENFORCE_ARG_COUNT(string.count, 1);
  ENFORCE_ARG_TYPE(string.count, 0, IS_STRING);

  b_obj_string *string = AS_STRING(METHOD_OBJECT);
  b_obj_string *substr = AS_STRING(args[0]);

  if (substr->length == 0 || string->length == 0)
    RETURN_NUMBER(0);

  int count = 0;
  const char *tmp = string->chars;
  while ((tmp = strstr(tmp, substr->chars))) {
    count++;
    tmp++;
  }

  RETURN_NUMBER(count);
}

DECLARE_STRING_METHOD(to_number) {
  ENFORCE_ARG_COUNT(string.to_number, 0);
  RETURN_NUMBER(strtod(AS_CSTRING(METHOD_OBJECT), NULL));
}

DECLARE_STRING_METHOD(to_list) {
  ENFORCE_ARG_COUNT(string.to_list, 0);
  b_obj_string *string = AS_STRING(METHOD_OBJECT);
  b_obj_list *list = new_list(vm);

  if (string->length > 0) {

    for (int i = 0; i < string->length; i++) {
      char *ch = &string->chars[i];
      write_value_arr(vm, &list->items, OBJ_VAL(copy_string(vm, ch, 1)));
    }
  }

  RETURN_OBJ(list);
}

DECLARE_STRING_METHOD(lpad) {
  ENFORCE_ARG_RANGE(string.lpad, 1, 2);
  ENFORCE_ARG_TYPE(string.lpad, 0, IS_NUMBER);

  b_obj_string *string = AS_STRING(METHOD_OBJECT);
  int width = AS_NUMBER(args[0]);
  char fill_char = ' ';

  if (arg_count == 2) {
    ENFORCE_ARG_TYPE(string.lpad, 1, IS_CHAR);
    fill_char = AS_CSTRING(args[1])[0];
  }

  if (width <= string->length)
    RETURN_VALUE(METHOD_OBJECT);

  int fill_size = width - string->length;
  char fill[fill_size];

  int i;
  for (i = 0; i < fill_size; i++)
    fill[i] = fill_char;

  char *str = ALLOCATE(char, string->length + fill_size + 1);
  memcpy(str, fill, fill_size);
  memcpy(str + fill_size, string->chars, string->length);
  str[string->length + fill_size] = '\0';

  RETURN_OBJ(copy_string(vm, str, string->length + fill_size));
}

DECLARE_STRING_METHOD(rpad) {
  ENFORCE_ARG_RANGE(string.rpad, 1, 2);
  ENFORCE_ARG_TYPE(string.rpad, 0, IS_NUMBER);

  b_obj_string *string = AS_STRING(METHOD_OBJECT);
  int width = AS_NUMBER(args[0]);
  char fill_char = ' ';

  if (arg_count == 2) {
    ENFORCE_ARG_TYPE(string.rpad, 1, IS_CHAR);
    fill_char = AS_CSTRING(args[1])[0];
  }

  if (width <= string->length)
    RETURN_VALUE(METHOD_OBJECT);

  int fill_size = width - string->length;
  char fill[fill_size];

  int i;
  for (i = 0; i < fill_size; i++)
    fill[i] = fill_char;

  char *str = ALLOCATE(char, string->length + fill_size + 1);
  memcpy(str, string->chars, string->length);
  memcpy(str + string->length, fill, fill_size);
  str[string->length + fill_size] = '\0';

  RETURN_OBJ(copy_string(vm, str, string->length + fill_size));
}

DECLARE_STRING_METHOD(match);
DECLARE_STRING_METHOD(matches);
DECLARE_STRING_METHOD(replace);