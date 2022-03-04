#ifndef BLADE_SSL_MODULE_H
#define BLADE_SSL_MODULE_H

#define DEFINE_SSL_CONSTANT(v) \
  b_value __ssl_##v(b_vm *vm) { \
    return NUMBER_VAL(v); \
  }

#define DEFINE_SSL_STR_CONSTANT(v) \
  b_value __ssl_##v(b_vm *vm) { \
    return STRING_VAL(v); \
  }

#define DEFINE_SSL_PTR_CONSTANT(v, j) \
  b_value __ssl_##v(b_vm *vm) { \
    return OBJ_VAL(new_ptr(vm, (void*)v j)); \
  }

#define GET_SSL_CONSTANT(v) \
  {#v, true, __ssl_##v}

#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/ssl.h>

#endif //BLADE_SSL_MODULE_H
