#include "module.h"
#include <zlib.h>
#include <assert.h>

#if defined(MSDOS) || defined(OS2) || defined(WIN32) || defined(__CYGWIN__)
#  include <fcntl.h>
#  include <io.h>
#  define SET_BINARY_MODE(file) setmode(fileno(file), O_BINARY)
#else
#  define SET_BINARY_MODE(file)
#endif

#define DEFINE_ZLIB_CONSTANT(v) \
  b_value __zlib_##v(b_vm *vm) { \
    return NUMBER_VAL(v); \
  }

#define GET_ZLIB_CONSTANT(v) \
  {#v, true, __zlib_##v}

#define CHECK_ERR(caller, err, msg) { \
  if (err != Z_OK) { \
    RETURN_ERROR(#caller "()::error(%d): " #err, msg); \
  }

// ZLIB_CHUNK_SIZE is 128kb
#define ZLIB_CHUNK_SIZE 131072

#define ZLIB_CHECK_RET_ERROR(fn, end) \
  switch (ret) { \
    case Z_OK: { \
      break; \
    } \
    case Z_MEM_ERROR: { \
      RETURN_ERROR(#fn "(): Out of memory while " #fn "ing data"); \
    } \
    case Z_DATA_ERROR: { \
      RETURN_ERROR(#fn "(): invalid or incomplete " #fn " data"); \
    } \
    case Z_STREAM_ERROR: { \
      RETURN_ERROR(#fn "(): Bad compression level"); \
    } \
    case Z_VERSION_ERROR: { \
      RETURN_ERROR(#fn "(): zlib version mismatch!"); \
    } \
    default: { \
      (void)end(&strm); \
      RETURN_ERROR(#fn "(): error while " #fn "ing data: %s (%d)", strm.msg, ret); \
    } \
  }

/* compression levels */
DEFINE_ZLIB_CONSTANT(Z_NO_COMPRESSION)
DEFINE_ZLIB_CONSTANT(Z_BEST_SPEED)
DEFINE_ZLIB_CONSTANT(Z_BEST_COMPRESSION)
DEFINE_ZLIB_CONSTANT(Z_DEFAULT_COMPRESSION)

/* compression strategy; see deflateInit2() below for details */
DEFINE_ZLIB_CONSTANT(Z_FILTERED)
DEFINE_ZLIB_CONSTANT(Z_HUFFMAN_ONLY)
DEFINE_ZLIB_CONSTANT(Z_RLE)
DEFINE_ZLIB_CONSTANT(Z_FIXED)
DEFINE_ZLIB_CONSTANT(Z_DEFAULT_STRATEGY)

// others
DEFINE_ZLIB_CONSTANT(MAX_WBITS)
b_value __zlib_Z_VERSION(b_vm *vm) {
  const char* version = zlibVersion();
  return STRING_VAL(version);
}



DECLARE_MODULE_METHOD(zlib_adler32) {
  ENFORCE_ARG_COUNT(adler32, 2);
  ENFORCE_ARG_TYPE(adler32, 0, IS_BYTES);
  ENFORCE_ARG_TYPE(adler32, 1, IS_NUMBER);

  uLong adler = (uLong) AS_NUMBER(args[1]); // default = 0

  b_obj_bytes *bytes = AS_BYTES(args[0]);
  RETURN_NUMBER(adler32(adler, (const Bytef *) bytes->bytes.bytes, (uInt) bytes->bytes.count));
}

DECLARE_MODULE_METHOD(zlib_crc32) {
  ENFORCE_ARG_COUNT(adler32, 2);
  ENFORCE_ARG_TYPE(adler32, 0, IS_BYTES);
  ENFORCE_ARG_TYPE(adler32, 1, IS_NUMBER);

  uLong crc = (uLong) AS_NUMBER(args[1]);

  b_obj_bytes *bytes = AS_BYTES(args[0]);
  RETURN_NUMBER(crc32(crc, (const Bytef *) bytes->bytes.bytes, (uInt) bytes->bytes.count));
}

static void get_buffer_remaining(z_stream *zst, uInt *remains) {
  uInt left = *remains;
  if(left > UINT_MAX) {
    left = UINT_MAX;
  }

  zst->avail_in = left;
  *remains -= zst->avail_in;
}

// deflate(data, level = -1)
DECLARE_MODULE_METHOD(zlib_deflate) {
  ENFORCE_ARG_COUNT(deflate, 5);
  ENFORCE_ARG_TYPE(deflate, 0, IS_BYTES);
  ENFORCE_ARG_TYPE(deflate, 1, IS_NUMBER);
  ENFORCE_ARG_TYPE(deflate, 2, IS_NUMBER);
  ENFORCE_ARG_TYPE(deflate, 3, IS_NUMBER);
  ENFORCE_ARG_TYPE(deflate, 4, IS_NUMBER);

  b_obj_bytes *b = AS_BYTES(args[0]);
  Byte * data = (Byte *)b->bytes.bytes;
  uInt data_length = b->bytes.count;

  int level = AS_NUMBER(args[1]);
  int strategy = AS_NUMBER(args[2]);
  int wbits = AS_NUMBER(args[3]);
  int memory_level = AS_NUMBER(args[4]);
  unsigned char out[ZLIB_CHUNK_SIZE];

  int ret, flush;
  unsigned have;
  z_stream strm;

  /* allocate deflate state */
  strm.zalloc = Z_NULL;
  strm.zfree = Z_NULL;
  strm.opaque = Z_NULL;
  ret = deflateInit2(&strm, level, Z_DEFLATED, wbits, memory_level, strategy);

  ZLIB_CHECK_RET_ERROR(deflate, deflateEnd)

  Bytef *output = (Byte *)calloc(0, sizeof(Bytef *));
  if(output == NULL) {
    RETURN_ERROR("deflate(): out of memory");
  }
  size_t output_length = 0;

  do {
    get_buffer_remaining(&strm, &data_length);
    flush = data_length == 0UL ? Z_FINISH : Z_NO_FLUSH;
    strm.next_in = data;

    do {
      strm.avail_out = ZLIB_CHUNK_SIZE;
      strm.next_out = out;

      ret = deflate(&strm, flush);
      if (ret == Z_STREAM_ERROR) {
        deflateEnd(&strm);
        RETURN_ERROR("deflate(): Error %d while deflating data", ret);
      }

      have = ZLIB_CHUNK_SIZE - strm.avail_out;

      if(have > 0) {
        size_t len = sizeof(Bytef) * have;
        output = (Bytef *) realloc(output, len + output_length);
        if(output == NULL) {
          RETURN_ERROR("deflate(): out of memory");
        }
        memcpy(output + output_length, out, have);
        output_length += len;
      }

    } while (strm.avail_out == 0);
    assert(strm.avail_in == 0);

  } while (flush != Z_FINISH);
  assert(ret == Z_STREAM_END);

  ret = deflateEnd(&strm);
  if(ret == Z_OK) {
    b_obj_bytes *bytes = new_bytes(vm, (int)output_length);
    bytes->bytes.bytes = (unsigned char *)output;
    RETURN_OBJ(bytes);
  }

  RETURN_ERROR("deflate(): Error %d while finishing deflation", ret);
}

// inflate(data, level = -1)
DECLARE_MODULE_METHOD(zlib_inflate) {
  ENFORCE_ARG_COUNT(inflate, 2);
  ENFORCE_ARG_TYPE(inflate, 0, IS_BYTES);
  ENFORCE_ARG_TYPE(inflate, 1, IS_NUMBER);

  b_obj_bytes *b = AS_BYTES(args[0]);
  Byte * data = (Byte *)b->bytes.bytes;
  uInt data_length = b->bytes.count;
  int wbits = AS_NUMBER(args[1]);

  unsigned char out[ZLIB_CHUNK_SIZE];

  int ret, flush;
  unsigned have;
  z_stream strm;

  /* allocate deflate state */
  strm.zalloc = Z_NULL;
  strm.zfree = Z_NULL;
  strm.opaque = Z_NULL;
  strm.avail_in = 0;
  strm.next_in = Z_NULL;
  ret = inflateInit2(&strm, wbits);

  ZLIB_CHECK_RET_ERROR(inflate, inflateEnd)

  Bytef *output = (Byte *)calloc(0, sizeof(Bytef *));
  if(output == NULL) {
    RETURN_ERROR("inflate(): out of memory");
  }
  size_t output_length = 0;

  do {
    get_buffer_remaining(&strm, &data_length);
    flush = data_length == 0UL ? Z_FINISH : Z_NO_FLUSH;
    strm.next_in = data;

    do {
      strm.avail_out = ZLIB_CHUNK_SIZE;
      strm.next_out = out;

      ret = inflate(&strm, flush);

      switch (ret) {
        case Z_OK:            /* fall through */
        case Z_BUF_ERROR:     /* fall through */
        case Z_STREAM_END:
          break;

        case Z_MEM_ERROR: {
          (void)inflateEnd(&strm);
          RETURN_ERROR("inflate(): out of memory");
        }
        default: {
          (void)inflateEnd(&strm);
          RETURN_ERROR("inflate(): error while inflating data: %s (%d)", strm.msg, ret);
        }
      }

      have = ZLIB_CHUNK_SIZE - strm.avail_out;

      if(have > 0) {
        size_t len = sizeof(Bytef) * have;
        output = (Bytef *) realloc(output, len + output_length);
        if(output == NULL) {
          RETURN_ERROR("inflate(): out of memory");
        }
        memcpy(output + output_length, out, have);
        output_length += len;
      }

    } while (strm.avail_out == 0);

  } while (ret != Z_STREAM_END);

  ret = inflateEnd(&strm);
  if(ret == Z_OK) {
    b_obj_bytes *bytes = new_bytes(vm, (int)output_length);
    bytes->bytes.bytes = (unsigned char *)output;
    RETURN_OBJ(bytes);
  }

  RETURN_ERROR("inflate(): Error %d while finishing inflation", ret);
}

CREATE_MODULE_LOADER(zlib) {
  static b_field_reg fields[] = {
      GET_ZLIB_CONSTANT(Z_NO_COMPRESSION),
      GET_ZLIB_CONSTANT(Z_BEST_SPEED),
      GET_ZLIB_CONSTANT(Z_BEST_COMPRESSION),
      GET_ZLIB_CONSTANT(Z_DEFAULT_COMPRESSION),

      GET_ZLIB_CONSTANT(Z_FILTERED),
      GET_ZLIB_CONSTANT(Z_HUFFMAN_ONLY),
      GET_ZLIB_CONSTANT(Z_RLE),
      GET_ZLIB_CONSTANT(Z_FIXED),
      GET_ZLIB_CONSTANT(Z_DEFAULT_STRATEGY),

      GET_ZLIB_CONSTANT(MAX_WBITS),

      /**
       * Blade extras...
       */
      GET_ZLIB_CONSTANT(Z_VERSION),

      {NULL,       false, NULL},
  };

  static b_func_reg module_functions[] = {
      {"adler32", false, GET_MODULE_METHOD(zlib_adler32)},
      {"crc32", false, GET_MODULE_METHOD(zlib_crc32)},
      {"deflate", false, GET_MODULE_METHOD(zlib_deflate)},
      {"inflate", false, GET_MODULE_METHOD(zlib_inflate)},
      {NULL,     false, NULL},
  };

  static b_module_reg module = {
      .name = "_zlib",
      .fields = fields,
      .functions = module_functions,
      .classes = NULL,
      .preloader = NULL,
      .unloader = NULL
  };

  return &module;
}