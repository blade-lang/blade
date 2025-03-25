#include "module.h"
#include "hash/md.h"
#include "hash/md5.h"
#include "hash/sha1.h"
#include "hash/sha256.h"
#include "hash/sha512.h"
#include "hash/whirlpool.h"
#include "hash/snefru.h"
#include "hash/fnv.h"
#include "hash/siphash.h"
#include "hash/gost.h"
#include "hash/crc32.h"
#include "hash/adler32.h"
#include "pathinfo.h"

#ifdef _MSC_VER
#define PRIx64 "llx"
#endif

DECLARE_MODULE_METHOD(hash__crc32) {
  ENFORCE_ARG_RANGE(crc32, 1, 2);
  ENFORCE_ARG_TYPES(crc32, 0, IS_STRING, IS_BYTES);

  uint32_t crc = 0;
  if (!IS_NIL(args[1])) {
    ENFORCE_ARG_TYPE(crc32, 1, IS_NUMBER);
    crc = (uint32_t) AS_NUMBER(args[1]);
  }

  if (IS_STRING(args[0])) {
    b_obj_string *string = AS_STRING(args[0]);
    RETURN_NUMBER(crc32(crc, (unsigned char *) string->chars, string->length));
  } else {
    b_obj_bytes *bytes = AS_BYTES(args[0]);
    RETURN_NUMBER(crc32(crc, bytes->bytes.bytes, bytes->bytes.count));
  }
}

DECLARE_MODULE_METHOD(hash__adler32) {
  ENFORCE_ARG_RANGE(adler32, 1, 2);
  ENFORCE_ARG_TYPES(adler32, 0, IS_STRING, IS_BYTES);

  unsigned int adler = 1;
  if (!IS_NIL(args[1])) {
    ENFORCE_ARG_TYPE(adler32, 1, IS_NUMBER);
    adler = (unsigned int) AS_NUMBER(args[1]);
  }

  if (IS_STRING(args[0])) {
    b_obj_string *string = AS_STRING(args[0]);
    RETURN_NUMBER(adler32(adler, (unsigned char *) string->chars, string->length));
  } else {
    b_obj_bytes *bytes = AS_BYTES(args[0]);
    RETURN_NUMBER(adler32(adler, bytes->bytes.bytes, bytes->bytes.count));
  }
}

DECLARE_MODULE_METHOD(hash__md2) {
  ENFORCE_ARG_COUNT(md2, 1);
  ENFORCE_ARG_TYPES(md2, 0, IS_STRING, IS_BYTES);

  char *result;
  if (IS_STRING(args[0])) {
    b_obj_string *string = AS_STRING(args[0]);
    result = MD2String((unsigned char *) string->chars, string->length);
  } else {
    b_obj_bytes *bytes = AS_BYTES(args[0]);
    result = MD2String(bytes->bytes.bytes, bytes->bytes.count);
  }

  RETURN_T_STRING(result, 32);
}

DECLARE_MODULE_METHOD(hash__md4) {
  ENFORCE_ARG_COUNT(md4, 1);
  ENFORCE_ARG_TYPES(md4, 0, IS_STRING, IS_BYTES);

  char *result;
  if (IS_STRING(args[0])) {
    b_obj_string *string = AS_STRING(args[0]);
    result = MD4String((unsigned char *) string->chars, string->length);
  } else {
    b_obj_bytes *bytes = AS_BYTES(args[0]);
    result = MD4String(bytes->bytes.bytes, bytes->bytes.count);
  }

  RETURN_T_STRING(result, 32);
}

DECLARE_MODULE_METHOD(hash__md5) {
  ENFORCE_ARG_COUNT(md5, 1);
  ENFORCE_ARG_TYPES(md5, 0, IS_STRING, IS_BYTES);

  char *result;
  if (IS_STRING(args[0])) {
    b_obj_string *string = AS_STRING(args[0]);
    result = MD5String((unsigned char *) string->chars, string->length);
  } else {
    b_obj_bytes *bytes = AS_BYTES(args[0]);
    result = MD5String(bytes->bytes.bytes, bytes->bytes.count);
  }

  RETURN_T_STRING(result, 32);
}

DECLARE_MODULE_METHOD(hash__md5_file) {
  ENFORCE_ARG_COUNT(md5_file, 1);
  ENFORCE_ARG_TYPE(md5_file, 0, IS_FILE);

  b_obj_file *file = AS_FILE(args[0]);

  if (file_exists(file->path->chars)) {
    char *result = MD5File(file->path->chars);
    if (result == NULL) {
      RETURN_ERROR("md5_file() could not open file");
    }

    RETURN_T_STRING(result, 32);
  }

  RETURN_ERROR("md5_file() file not found");
}

DECLARE_MODULE_METHOD(hash__sha1) {
  ENFORCE_ARG_COUNT(sha1, 1);
  ENFORCE_ARG_TYPES(sha1, 0, IS_STRING, IS_BYTES);

  char *result;
  if (IS_STRING(args[0])) {
    b_obj_string *string = AS_STRING(args[0]);
    result = SHA1String((unsigned char *) string->chars, string->length);
  } else {
    b_obj_bytes *bytes = AS_BYTES(args[0]);
    result = SHA1String(bytes->bytes.bytes, bytes->bytes.count);
  }

  RETURN_T_STRING(result, 40);
}

DECLARE_MODULE_METHOD(hash__sha224) {
  ENFORCE_ARG_COUNT(sha224, 1);
  ENFORCE_ARG_TYPES(sha224, 0, IS_STRING, IS_BYTES);

  char *result;
  if (IS_STRING(args[0])) {
    b_obj_string *string = AS_STRING(args[0]);
    result = sha224_string((unsigned char *) string->chars, string->length);
  } else {
    b_obj_bytes *bytes = AS_BYTES(args[0]);
    result = sha224_string(bytes->bytes.bytes, bytes->bytes.count);
  }

  b_obj_string *string = copy_string(vm, result, 56);
  free(result);
  RETURN_OBJ(string);
}

DECLARE_MODULE_METHOD(hash__sha256) {
  ENFORCE_ARG_COUNT(sha256, 1);
  ENFORCE_ARG_TYPES(sha256, 0, IS_STRING, IS_BYTES);

  char *result;
  if (IS_STRING(args[0])) {
    b_obj_string *string = AS_STRING(args[0]);
    result = sha256_string((unsigned char *) string->chars, string->length);
  } else {
    b_obj_bytes *bytes = AS_BYTES(args[0]);
    result = sha256_string(bytes->bytes.bytes, bytes->bytes.count);
  }

  RETURN_T_STRING(result, 64);
}

DECLARE_MODULE_METHOD(hash__sha384) {
  ENFORCE_ARG_COUNT(sha384, 1);
  ENFORCE_ARG_TYPES(sha384, 0, IS_STRING, IS_BYTES);

  char *result;
  if (IS_STRING(args[0])) {
    b_obj_string *string = AS_STRING(args[0]);
    result = SHA384String((unsigned char *) string->chars, string->length);
  } else {
    b_obj_bytes *bytes = AS_BYTES(args[0]);
    result = SHA384String(bytes->bytes.bytes, bytes->bytes.count);
  }

  b_obj_string *string = copy_string(vm, result, 96);
  free(result);
  RETURN_OBJ(string);
}

DECLARE_MODULE_METHOD(hash__sha512) {
  ENFORCE_ARG_COUNT(sha512, 1);
  ENFORCE_ARG_TYPES(sha512, 0, IS_STRING, IS_BYTES);

  char *result;
  if (IS_STRING(args[0])) {
    b_obj_string *string = AS_STRING(args[0]);
    result = SHA512String((unsigned char *) string->chars, string->length);
  } else {
    b_obj_bytes *bytes = AS_BYTES(args[0]);
    result = SHA512String(bytes->bytes.bytes, bytes->bytes.count);
  }

  RETURN_T_STRING(result, 128);
}

DECLARE_MODULE_METHOD(hash__fnv1) {
  ENFORCE_ARG_COUNT(fnv1, 1);
  ENFORCE_ARG_TYPES(fnv1, 0, IS_STRING, IS_BYTES);

  char *result;
  if (IS_STRING(args[0])) {
    b_obj_string *string = AS_STRING(args[0]);
    result = FNV1((unsigned char *) string->chars, string->length);
  } else {
    b_obj_bytes *bytes = AS_BYTES(args[0]);
    result = FNV1(bytes->bytes.bytes, bytes->bytes.count);
  }

  RETURN_TT_STRING(result);
}

DECLARE_MODULE_METHOD(hash__fnv1a) {
  ENFORCE_ARG_COUNT(fnv1a, 1);
  ENFORCE_ARG_TYPES(fnv1a, 0, IS_STRING, IS_BYTES);

  char *result;
  if (IS_STRING(args[0])) {
    b_obj_string *string = AS_STRING(args[0]);
    result = FNV1a((unsigned char *) string->chars, string->length);
  } else {
    b_obj_bytes *bytes = AS_BYTES(args[0]);
    result = FNV1a(bytes->bytes.bytes, bytes->bytes.count);
  }

  RETURN_TT_STRING(result);
}

DECLARE_MODULE_METHOD(hash__fnv1_64) {
  ENFORCE_ARG_COUNT(fnv1_64, 1);
  ENFORCE_ARG_TYPES(fnv1_64, 0, IS_STRING, IS_BYTES);

  char *result;
  if (IS_STRING(args[0])) {
    b_obj_string *string = AS_STRING(args[0]);
    result = FNV164((unsigned char *) string->chars, string->length);
  } else {
    b_obj_bytes *bytes = AS_BYTES(args[0]);
    result = FNV164(bytes->bytes.bytes, bytes->bytes.count);
  }

  RETURN_TT_STRING(result);
}

DECLARE_MODULE_METHOD(hash__fnv1a_64) {
  ENFORCE_ARG_COUNT(fnv1a64, 1);
  ENFORCE_ARG_TYPES(fnv1a64, 0, IS_STRING, IS_BYTES);

  char *result;
  if (IS_STRING(args[0])) {
    b_obj_string *string = AS_STRING(args[0]);
    result = FNV1a64((unsigned char *) string->chars, string->length);
  } else {
    b_obj_bytes *bytes = AS_BYTES(args[0]);
    result = FNV1a64(bytes->bytes.bytes, bytes->bytes.count);
  }

  RETURN_TT_STRING(result);
}

DECLARE_MODULE_METHOD(hash__whirlpool) {
  ENFORCE_ARG_COUNT(whirlpool, 1);
  ENFORCE_ARG_TYPES(whirlpool, 0, IS_STRING, IS_BYTES);

  char *result;
  if (IS_STRING(args[0])) {
    b_obj_string *string = AS_STRING(args[0]);
    result = WhirlpoolString((unsigned char *) string->chars, string->length);
  } else {
    b_obj_bytes *bytes = AS_BYTES(args[0]);
    result = WhirlpoolString(bytes->bytes.bytes, bytes->bytes.count);
  }

  RETURN_TT_STRING(result);
}

DECLARE_MODULE_METHOD(hash__snefru) {
  ENFORCE_ARG_COUNT(snefru, 1);
  ENFORCE_ARG_TYPES(snefru, 0, IS_STRING, IS_BYTES);

  char *result;
  if (IS_STRING(args[0])) {
    b_obj_string *string = AS_STRING(args[0]);
    result = SnefruString((unsigned char *) string->chars, string->length);
  } else {
    b_obj_bytes *bytes = AS_BYTES(args[0]);
    result = SnefruString(bytes->bytes.bytes, bytes->bytes.count);
  }

  RETURN_TT_STRING(result);
}

DECLARE_MODULE_METHOD(hash__siphash) {
  ENFORCE_ARG_COUNT(siphash, 2);
  ENFORCE_ARG_TYPE(siphash, 0, IS_BYTES);
  ENFORCE_ARG_TYPE(siphash, 1, IS_BYTES);

  b_obj_bytes *key = AS_BYTES(args[0]);
  b_obj_bytes *str = AS_BYTES(args[1]);

  uint64_t sip = siphash24(str->bytes.bytes, str->bytes.count, (const char *) key->bytes.bytes);

  char result[17]; // assume maximum of 16 bits
  int length = sprintf(result, "%" PRIx64, sip);

  RETURN_L_STRING(result, length);
}

DECLARE_MODULE_METHOD(hash__gost) {
  ENFORCE_ARG_COUNT(gost, 1);
  ENFORCE_ARG_TYPES(gost, 0, IS_STRING, IS_BYTES);

  char *result;
  if (IS_STRING(args[0])) {
    b_obj_string *string = AS_STRING(args[0]);
    result = GOSTString((unsigned char *) string->chars, string->length);
  } else {
    b_obj_bytes *bytes = AS_BYTES(args[0]);
    result = GOSTString(bytes->bytes.bytes, bytes->bytes.count);
  }

  RETURN_TT_STRING(result);
}


DECLARE_MODULE_METHOD(hash__hash) {
  ENFORCE_ARG_COUNT(hash, 1);
  METHOD_OVERRIDE(hash, 4);
  RETURN_NUMBER((double) hash_value(args[0]));
}

CREATE_MODULE_LOADER(hash) {
  static b_func_reg module_functions[] = {
      {"hash",      true,  GET_MODULE_METHOD(hash__hash)},
      {"adler32",   true,  GET_MODULE_METHOD(hash__adler32)},
      {"crc32",     true,  GET_MODULE_METHOD(hash__crc32)},
      {"md2",       true,  GET_MODULE_METHOD(hash__md2)},
      {"md4",       true,  GET_MODULE_METHOD(hash__md4)},
      {"md5",       true,  GET_MODULE_METHOD(hash__md5)},
      {"md5_file",  true,  GET_MODULE_METHOD(hash__md5_file)},
      {"sha1",      true,  GET_MODULE_METHOD(hash__sha1)},
      {"sha224",    true,  GET_MODULE_METHOD(hash__sha224)},
      {"sha256",    true,  GET_MODULE_METHOD(hash__sha256)},
      {"sha384",    true,  GET_MODULE_METHOD(hash__sha384)},
      {"sha512",    true,  GET_MODULE_METHOD(hash__sha512)},
      {"fnv1",      true,  GET_MODULE_METHOD(hash__fnv1)},
      {"fnv1a",     true,  GET_MODULE_METHOD(hash__fnv1a)},
      {"fnv1_64",   true,  GET_MODULE_METHOD(hash__fnv1_64)},
      {"fnv1a_64",  true,  GET_MODULE_METHOD(hash__fnv1a_64)},
      {"whirlpool", true,  GET_MODULE_METHOD(hash__whirlpool)},
      {"snefru",    true,  GET_MODULE_METHOD(hash__snefru)},
      {"siphash",   true,  GET_MODULE_METHOD(hash__siphash)},
      {"gost",      true,  GET_MODULE_METHOD(hash__gost)},
      {NULL,        false, NULL},
  };

  static b_module_reg module = {
      .name = "_hash",
      .fields = NULL,
      .functions = module_functions,
      .classes = NULL,
      .preloader = NULL,
      .unloader = NULL
  };

  return &module;
}