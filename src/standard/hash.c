#include "hash.h"
#include "hash/md5.h"
#include "hash/crc32.h"
#include "pathinfo.h"

DECLARE_MODULE_METHOD(hash__crc32) {
  ENFORCE_ARG_RANGE(crc32, 1, 2);

  if(!IS_STRING(args[0]) && !IS_BYTES(args[0])){
    RETURN_ERROR("crc32() expects string or bytes");
  }

  uint32_t crc = 0;
  if(!IS_NIL(args[1])){
    ENFORCE_ARG_TYPE(crc32, 0, IS_NUMBER);
    crc = (uint32_t) AS_NUMBER(args[1]);
  }

  if(IS_STRING(args[0])){
    b_obj_string *string = AS_STRING(args[0]);
    RETURN_NUMBER(crc32(crc, (unsigned char *)string->chars, string->length));
  } else {
    b_obj_bytes *bytes = AS_BYTES(args[0]);
    RETURN_NUMBER(crc32(crc, bytes->bytes.bytes, bytes->bytes.count));
  }
}

DECLARE_MODULE_METHOD(hash__md5) {
  ENFORCE_ARG_COUNT(md5, 1);

  if(!IS_STRING(args[0]) && !IS_BYTES(args[0])){
    RETURN_ERROR("md5() expects string or bytes");
  }

  if(IS_STRING(args[0])){
    b_obj_string *string = AS_STRING(args[0]);
    char *result = MD5String(string->chars, string->length);
    RETURN_T_STRING(result, 32);
  } else {
    b_obj_bytes *bytes = AS_BYTES(args[0]);
    char *result = MD5String((char *)bytes->bytes.bytes, bytes->bytes.count);
    RETURN_T_STRING(result, 32);
  }
}

DECLARE_MODULE_METHOD(hash__md5_file) {
  ENFORCE_ARG_COUNT(md5_file, 1);
  ENFORCE_ARG_TYPE(md5_file, 0, IS_FILE);

  b_obj_file *file = AS_FILE(args[0]);

  if(file_exists(file->path->chars)) {
    char *result = MD5File(file->path->chars);
    if(result == NULL) {
      RETURN_ERROR("md5_file() could not open file");
    }

    RETURN_T_STRING(result, 32);
  }

  RETURN_ERROR("md5_file() file not found");
}

CREATE_MODULE_LOADER(hash) {
  static b_func_reg class_functions[] = {
      {"crc32", true, GET_MODULE_METHOD(hash__crc32)},
      {"md5", true, GET_MODULE_METHOD(hash__md5)},
      {"md5_file", true, GET_MODULE_METHOD(hash__md5_file)},
      {NULL,      false, NULL},
  };

  static b_class_reg classes[] = {
      {"Hash", NULL, class_functions},
      {NULL,     NULL, NULL},
  };

  static b_module_reg module = {NULL, classes};

  return module;
}