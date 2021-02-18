#include "builtin/bytes.h"
#include "memory.h"

#include <ctype.h>
#include <string.h>

DECLARE_NATIVE(bytes) {
  ENFORCE_ARG_COUNT(bytes, 1);
  if (IS_NUMBER(args[0])) {
    b_obj_bytes *bytes = new_bytes(vm, (int)AS_NUMBER(args[0]));
    RETURN_OBJ(bytes);
  } else if (IS_LIST(args[0])) {
    b_obj_list *list = AS_LIST(args[0]);
    b_obj_bytes *bytes = new_bytes(vm, list->items.count);

    for (int i = 0; i < list->items.count; i++) {
      if (!IS_NUMBER(list->items.values[i])) {
        RETURN_ERROR("bytes() expects a list of valid bytes");
      }

      int byte = AS_NUMBER(list->items.values[i]);

      if (byte < 0 || byte > 255) {
        RETURN_ERROR("invalid byte. bytes range from 0 to 255");
      }

      bytes->bytes.bytes[i] = (unsigned char)byte;
    }

    RETURN_OBJ(bytes);
  }

  RETURN_ERROR("expected array size of bytes list as argument");
}

DECLARE_BYTES_METHOD(length) {
  ENFORCE_ARG_COUNT(length, 0);
  RETURN_NUMBER(AS_BYTES(METHOD_OBJECT)->bytes.count);
}

DECLARE_BYTES_METHOD(append) {
  ENFORCE_ARG_COUNT(append, 1);

  if (IS_NUMBER(args[0])) {
    int byte = (int)AS_NUMBER(args[0]);
    if (byte < 0 || byte > 255) {
      RETURN_ERROR("invalid byte. bytes range from 0 to 255");
    }

    // append here...
    b_obj_bytes *bytes = AS_BYTES(METHOD_OBJECT);
    bytes->bytes.bytes = reallocate(vm, bytes->bytes.bytes, bytes->bytes.count,
                                    bytes->bytes.count++);
    bytes->bytes.bytes[bytes->bytes.count - 1] = (unsigned char)byte;
    RETURN;
  } else if (IS_LIST(args[0])) {
    b_obj_list *list = AS_LIST(args[0]);
    if (list->items.count > 0) {
      // append here...
      b_obj_bytes *bytes = AS_BYTES(METHOD_OBJECT);
      bytes->bytes.bytes =
          reallocate(vm, bytes->bytes.bytes, bytes->bytes.count,
                     bytes->bytes.count + list->items.count);

      for (int i = 0; i < list->items.count; i++) {
        if (!IS_NUMBER(list->items.values[i])) {
          RETURN_ERROR("bytes lists can only contain numbers");
        }

        int byte = (int)AS_NUMBER(list->items.values[i]);
        if (byte < 0 || byte > 255) {
          RETURN_ERROR("invalid byte. bytes range from 0 to 255");
        }

        bytes->bytes.bytes[bytes->bytes.count + i] = (unsigned char)byte;
      }

      bytes->bytes.count += list->items.count;
    }
    RETURN;
  }

  RETURN_ERROR("bytes can only append a byte or a list of bytes");
}

DECLARE_BYTES_METHOD(clone) {
  ENFORCE_ARG_COUNT(clone, 0);
  b_obj_bytes *bytes = AS_BYTES(METHOD_OBJECT);
  b_obj_bytes *nbytes = new_bytes(vm, bytes->bytes.count);

  memcpy(nbytes->bytes.bytes, bytes->bytes.bytes, bytes->bytes.count);

  RETURN_OBJ(nbytes);
}

DECLARE_BYTES_METHOD(extend) {
  ENFORCE_ARG_COUNT(extend, 1);
  ENFORCE_ARG_TYPE(extend, 0, IS_BYTES);
  b_obj_bytes *bytes = AS_BYTES(METHOD_OBJECT);
  b_obj_bytes *nbytes = AS_BYTES(args[0]);

  bytes->bytes.bytes = reallocate(vm, bytes->bytes.bytes, bytes->bytes.count,
                                  bytes->bytes.count + nbytes->bytes.count);

  memcpy(bytes->bytes.bytes + bytes->bytes.count, nbytes->bytes.bytes,
         nbytes->bytes.count);
  bytes->bytes.count += nbytes->bytes.count;
  RETURN;
}

DECLARE_BYTES_METHOD(pop) {
  ENFORCE_ARG_COUNT(pop, 0);
  b_obj_bytes *bytes = AS_BYTES(METHOD_OBJECT);
  unsigned char c = bytes->bytes.bytes[bytes->bytes.count - 1];
  bytes->bytes.count--;
  RETURN_NUMBER((double)((int)c));
}

DECLARE_BYTES_METHOD(remove) {
  ENFORCE_ARG_COUNT(remove, 1);
  ENFORCE_ARG_TYPE(remove, 0, IS_NUMBER);

  b_obj_bytes *bytes = AS_BYTES(METHOD_OBJECT);
  int index = AS_NUMBER(args[0]);

  if (index < 0 || index >= bytes->bytes.count) {
    RETURN_ERROR("bytes index %d out of range", index);
  }

  unsigned char val = bytes->bytes.bytes[index];

  for (int i = index; i < bytes->bytes.count; i++) {
    bytes->bytes.bytes[i] = bytes->bytes.bytes[i + 1];
  }
  bytes->bytes.count--;

  RETURN_NUMBER((double)((int)val));
}

DECLARE_BYTES_METHOD(reverse) {
  ENFORCE_ARG_COUNT(reverse, 0);
  b_obj_bytes *bytes = AS_BYTES(METHOD_OBJECT);

  b_obj_bytes *nbytes = new_bytes(vm, bytes->bytes.count);

  for (int i = 0; i < bytes->bytes.count; i++) {
    nbytes->bytes.bytes[i] = bytes->bytes.bytes[bytes->bytes.count - i - 1];
  }

  RETURN_OBJ(nbytes);
}

DECLARE_BYTES_METHOD(first) {
  ENFORCE_ARG_COUNT(first, 0);
  RETURN_NUMBER((double)((int)AS_BYTES(METHOD_OBJECT)->bytes.bytes[0]));
}

DECLARE_BYTES_METHOD(last) {
  ENFORCE_ARG_COUNT(first, 0);
  b_obj_bytes *bytes = AS_BYTES(METHOD_OBJECT);
  RETURN_NUMBER((double)((int)bytes->bytes.bytes[bytes->bytes.count - 1]));
}

DECLARE_BYTES_METHOD(get) {
  ENFORCE_ARG_COUNT(get, 1);
  ENFORCE_ARG_TYPE(get, 0, IS_NUMBER);

  b_obj_bytes *bytes = AS_BYTES(METHOD_OBJECT);
  int index = AS_NUMBER(args[0]);
  if (index < 0 || index >= bytes->bytes.count) {
    RETURN_ERROR("bytes index %d out of range", index);
  }

  RETURN_NUMBER((double)((int)bytes->bytes.bytes[index]));
}

DECLARE_BYTES_METHOD(is_alpha) {
  ENFORCE_ARG_COUNT(is_alpha, 0);
  b_obj_bytes *bytes = AS_BYTES(METHOD_OBJECT);

  for (int i = 0; i < bytes->bytes.count; i++) {
    if (!isalpha(bytes->bytes.bytes[i]))
      RETURN_FALSE;
  }
  RETURN_TRUE;
}

DECLARE_BYTES_METHOD(is_alnum) {
  ENFORCE_ARG_COUNT(is_alnum, 0);
  b_obj_bytes *bytes = AS_BYTES(METHOD_OBJECT);

  for (int i = 0; i < bytes->bytes.count; i++) {
    if (!isalnum(bytes->bytes.bytes[i]))
      RETURN_FALSE;
  }
  RETURN_TRUE;
}

DECLARE_BYTES_METHOD(is_number) {
  ENFORCE_ARG_COUNT(is_number, 0);
  b_obj_bytes *bytes = AS_BYTES(METHOD_OBJECT);

  for (int i = 0; i < bytes->bytes.count; i++) {
    if (!isdigit(bytes->bytes.bytes[i]))
      RETURN_FALSE;
  }
  RETURN_TRUE;
}

DECLARE_BYTES_METHOD(is_lower) {
  ENFORCE_ARG_COUNT(is_lower, 0);
  b_obj_bytes *bytes = AS_BYTES(METHOD_OBJECT);

  for (int i = 0; i < bytes->bytes.count; i++) {
    if (!islower(bytes->bytes.bytes[i]))
      RETURN_FALSE;
  }
  RETURN_TRUE;
}

DECLARE_BYTES_METHOD(is_upper) {
  ENFORCE_ARG_COUNT(is_upper, 0);
  b_obj_bytes *bytes = AS_BYTES(METHOD_OBJECT);

  for (int i = 0; i < bytes->bytes.count; i++) {
    if (!isupper(bytes->bytes.bytes[i]))
      RETURN_FALSE;
  }
  RETURN_TRUE;
}

DECLARE_BYTES_METHOD(is_space) {
  ENFORCE_ARG_COUNT(is_space, 0);
  b_obj_bytes *bytes = AS_BYTES(METHOD_OBJECT);

  for (int i = 0; i < bytes->bytes.count; i++) {
    if (!isspace(bytes->bytes.bytes[i]))
      RETURN_FALSE;
  }
  RETURN_TRUE;
}

DECLARE_BYTES_METHOD(to_list) {
  ENFORCE_ARG_COUNT(to_list, 0);
  b_obj_bytes *bytes = AS_BYTES(METHOD_OBJECT);
  b_obj_list *list = new_list(vm);

  for (int i = 0; i < bytes->bytes.count; i++) {
    write_list(vm, list, NUMBER_VAL((double)((int)bytes->bytes.bytes[i])));
  }

  RETURN_OBJ(list);
}

DECLARE_BYTES_METHOD(to_string) {
  ENFORCE_ARG_COUNT(to_string, 0);
  b_obj_bytes *bytes = AS_BYTES(METHOD_OBJECT);

  char *string = (char *)bytes->bytes.bytes;
  RETURN_LSTRING(string, bytes->bytes.count);
}

DECLARE_BYTES_METHOD(__iter__) {
  ENFORCE_ARG_COUNT(__iter__, 1);
  ENFORCE_ARG_TYPE(__iter__, 0, IS_NUMBER);

  b_obj_bytes *bytes = AS_BYTES(METHOD_OBJECT);

  int index = AS_NUMBER(args[0]);

  if (index > -1 && index < bytes->bytes.count) {
    RETURN_NUMBER((int)bytes->bytes.bytes[index]);
  }

  RETURN;
}

DECLARE_BYTES_METHOD(__itern__) {
  ENFORCE_ARG_COUNT(__itern__, 1);
  b_obj_bytes *bytes = AS_BYTES(METHOD_OBJECT);

  if (IS_NIL(args[0])) {
    if (bytes->bytes.count == 0)
      RETURN_FALSE;
    RETURN_NUMBER(0);
  }

  if (!IS_NUMBER(args[0])) {
    RETURN_ERROR("bytes are numerically indexed");
  }

  int index = AS_NUMBER(args[0]);
  if (index < bytes->bytes.count - 1) {
    RETURN_NUMBER(index + 1);
  }

  RETURN;
}