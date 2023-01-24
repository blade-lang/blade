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

#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/ssl.h>

#endif //BLADE_SSL_MODULE_H
