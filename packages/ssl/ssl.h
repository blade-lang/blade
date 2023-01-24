#ifndef BLADE_SSL_MODULE_H
#define BLADE_SSL_MODULE_H

#define DEFINE_SSL_CONSTANT(v) \
  DECLARE_MODULE_METHOD(ssl_const_##v) { \
    RETURN_NUMBER(v); \
  }

#define DEFINE_SSL_STR_CONSTANT(v) \
  DECLARE_MODULE_METHOD(ssl_const_##v) { \
    RETURN_STRING(v); \
  }

#define DEFINE_SSL_PTR_CONSTANT(v) \
  DECLARE_MODULE_METHOD(ssl_const_##v) { \
    RETURN_PTR(v()); \
  }

#define GET_SSL_CONSTANT(v) \
  {#v, true, GET_MODULE_METHOD(ssl_const_##v)}

#define RETURN_SSL_ERROR() \
  unsigned long e = ERR_get_error(); \
  char *err = ERR_error_string(e, NULL); \
  RETURN_ERROR("%s: %s", err, ERR_reason_error_string(e))

#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/ssl.h>

#endif //BLADE_SSL_MODULE_H
