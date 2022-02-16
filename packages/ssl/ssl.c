#include <blade.h>
#include <unistd.h>
#include "ssl.h"

DEFINE_SSL_CONSTANT(SSL_FILETYPE_PEM)
DEFINE_SSL_CONSTANT(SSL_FILETYPE_ASN1)

DEFINE_SSL_CONSTANT(SSL_VERIFY_NONE)
DEFINE_SSL_CONSTANT(SSL_VERIFY_PEER)
DEFINE_SSL_CONSTANT(SSL_VERIFY_FAIL_IF_NO_PEER_CERT)
DEFINE_SSL_CONSTANT(SSL_VERIFY_CLIENT_ONCE)
DEFINE_SSL_CONSTANT(SSL_VERIFY_POST_HANDSHAKE)

DEFINE_SSL_CONSTANT(BIO_CLOSE)
DEFINE_SSL_CONSTANT(BIO_NOCLOSE)

DEFINE_SSL_PTR_CONSTANT(TLS_method,())
DEFINE_SSL_PTR_CONSTANT(TLS_client_method,())
DEFINE_SSL_PTR_CONSTANT(TLS_server_method,())
DEFINE_SSL_PTR_CONSTANT(SSLv23_method,())
DEFINE_SSL_PTR_CONSTANT(SSLv23_client_method,())
DEFINE_SSL_PTR_CONSTANT(SSLv23_server_method,())

DEFINE_SSL_PTR_CONSTANT(BIO_f_ssl,())
DEFINE_SSL_PTR_CONSTANT(BIO_s_connect,())
DEFINE_SSL_PTR_CONSTANT(BIO_s_accept,())

DECLARE_MODULE_METHOD(ssl_ctx) {
  ENFORCE_ARG_COUNT(ctx, 1);
  ENFORCE_ARG_TYPE(ctx, 0, IS_PTR);
  RETURN_PTR(SSL_CTX_new((SSL_METHOD*) AS_PTR(args[0])->pointer));
}

DECLARE_MODULE_METHOD(ssl_ctx_free) {
  ENFORCE_ARG_COUNT(ctx_free, 1);
  ENFORCE_ARG_TYPE(ctx_free, 0, IS_PTR);
  SSL_CTX_free((SSL_CTX*)AS_PTR(args[0])->pointer);
  RETURN;
}

// @TODO: Treat the callback in the client code
DECLARE_MODULE_METHOD(ssl_ctx_set_verify) {
  ENFORCE_ARG_COUNT(ctx_set_verify, 2);
  ENFORCE_ARG_TYPE(ctx_set_verify, 0, IS_PTR);
  ENFORCE_ARG_TYPE(ctx_set_verify, 1, IS_NUMBER);

  SSL_CTX *ctx = (SSL_CTX*)AS_PTR(args[0])->pointer;
  SSL_CTX_set_verify(ctx, AS_NUMBER(args[1]), NULL);
  RETURN;
}

// @TODO: Treat the callback in the client code
DECLARE_MODULE_METHOD(ssl_ctx_set_verify_locations) {
  ENFORCE_ARG_COUNT(ctx_set_verify_locations, 2);
  ENFORCE_ARG_TYPE(ctx_set_verify_locations, 0, IS_PTR);
  ENFORCE_ARG_TYPE(ctx_set_verify_locations, 1, IS_STRING);

  SSL_CTX *ctx = (SSL_CTX*)AS_PTR(args[0])->pointer;
  RETURN_BOOL(SSL_CTX_load_verify_locations(ctx, AS_C_STRING(args[1]), NULL) == 1);
}

DECLARE_MODULE_METHOD(ssl_new) {
  ENFORCE_ARG_COUNT(new, 1);
  ENFORCE_ARG_TYPE(new, 0, IS_PTR);

  SSL_CTX *ctx = (SSL_CTX*)AS_PTR(args[0])->pointer;
  RETURN_PTR(SSL_new(ctx));
}

DECLARE_MODULE_METHOD(ssl_ssl_free) {
  ENFORCE_ARG_COUNT(ssl_free, 1);
  ENFORCE_ARG_TYPE(ssl_free, 0, IS_PTR);

  SSL *ssl = (SSL*)AS_PTR(args[0])->pointer;
  SSL_free(ssl);
  RETURN;
}

DECLARE_MODULE_METHOD(ssl_set_connect_state) {
  ENFORCE_ARG_COUNT(set_connect_state, 1);
  ENFORCE_ARG_TYPE(set_connect_state, 0, IS_PTR);

  SSL *ssl = (SSL*)AS_PTR(args[0])->pointer;
  SSL_set_connect_state(ssl);
  RETURN;
}

DECLARE_MODULE_METHOD(ssl_new_bio) {
  ENFORCE_ARG_COUNT(new_bio, 1);
  ENFORCE_ARG_TYPE(new_bio, 0, IS_PTR);

  BIO_METHOD *type = (BIO_METHOD*)AS_PTR(args[0])->pointer;
  RETURN_PTR(BIO_new(type));
}

DECLARE_MODULE_METHOD(ssl_bio_set_ssl) {
  ENFORCE_ARG_COUNT(bio_set_ssl, 3);
  ENFORCE_ARG_TYPE(bio_set_ssl, 0, IS_PTR); //bio
  ENFORCE_ARG_TYPE(bio_set_ssl, 1, IS_PTR); // ssl
  ENFORCE_ARG_TYPE(bio_set_ssl, 2, IS_NUMBER); // args

  BIO *bio = (BIO*)AS_PTR(args[0])->pointer;
  SSL *ssl = (SSL*)AS_PTR(args[1])->pointer;
  BIO_set_ssl(bio, ssl, (int)AS_NUMBER(args[2]));
  RETURN;
}

DECLARE_MODULE_METHOD(ssl_set_conn_hostname) {
  ENFORCE_ARG_COUNT(set_conn_hostname, 2);
  ENFORCE_ARG_TYPE(set_conn_hostname, 0, IS_PTR);
  ENFORCE_ARG_TYPE(set_conn_hostname, 1, IS_STRING);

  BIO *bio = (BIO*)AS_PTR(args[0])->pointer;
  BIO_set_conn_hostname(bio, AS_C_STRING(args[1]));
  RETURN;
}

DECLARE_MODULE_METHOD(ssl_set_accept_name) {
  ENFORCE_ARG_COUNT(set_accept_name, 2);
  ENFORCE_ARG_TYPE(set_accept_name, 0, IS_PTR);
  ENFORCE_ARG_TYPE(set_accept_name, 1, IS_STRING);

  BIO *bio = (BIO*)AS_PTR(args[0])->pointer;
  BIO_set_accept_name(bio, AS_C_STRING(args[1]));
  RETURN;
}

DECLARE_MODULE_METHOD(ssl_set_conn_address) {
  ENFORCE_ARG_COUNT(set_conn_address, 2);
  ENFORCE_ARG_TYPE(set_conn_address, 0, IS_PTR);
  ENFORCE_ARG_TYPE(set_conn_address, 1, IS_STRING);

  BIO *bio = (BIO*)AS_PTR(args[0])->pointer;
  BIO_set_conn_address(bio, AS_C_STRING(args[1]));
  RETURN;
}

DECLARE_MODULE_METHOD(ssl_set_conn_port) {
  ENFORCE_ARG_COUNT(set_conn_port, 2);
  ENFORCE_ARG_TYPE(set_conn_port, 0, IS_PTR);
  ENFORCE_ARG_TYPE(set_conn_port, 1, IS_STRING);

  BIO *bio = (BIO*)AS_PTR(args[0])->pointer;
  BIO_set_conn_port(bio, AS_C_STRING(args[1]));
  RETURN;
}

DECLARE_MODULE_METHOD(ssl_set_accept_port) {
  ENFORCE_ARG_COUNT(set_accept_port, 2);
  ENFORCE_ARG_TYPE(set_accept_port, 0, IS_PTR);
  ENFORCE_ARG_TYPE(set_accept_port, 1, IS_STRING);

  BIO *bio = (BIO*)AS_PTR(args[0])->pointer;
  BIO_set_accept_port(bio, AS_C_STRING(args[1]));
  RETURN;
}

DECLARE_MODULE_METHOD(ssl_set_conn_family) {
  ENFORCE_ARG_COUNT(set_conn_family, 2);
  ENFORCE_ARG_TYPE(set_conn_family, 0, IS_PTR);
  ENFORCE_ARG_TYPE(set_conn_family, 1, IS_NUMBER);

  BIO *bio = (BIO*)AS_PTR(args[0])->pointer;
  BIO_set_conn_ip_family(bio, AS_NUMBER(args[1]));
  RETURN;
}

DECLARE_MODULE_METHOD(ssl_set_accept_family) {
  ENFORCE_ARG_COUNT(set_accept_family, 2);
  ENFORCE_ARG_TYPE(set_accept_family, 0, IS_PTR);
  ENFORCE_ARG_TYPE(set_accept_family, 1, IS_NUMBER);

  BIO *bio = (BIO*)AS_PTR(args[0])->pointer;
  BIO_set_accept_ip_family(bio, AS_NUMBER(args[1]));
  RETURN;
}

DECLARE_MODULE_METHOD(ssl_get_conn_hostname) {
  ENFORCE_ARG_COUNT(get_conn_hostname, 1);
  ENFORCE_ARG_TYPE(get_conn_hostname, 0, IS_PTR);
  char *hostname = (char *)BIO_get_conn_hostname((BIO*)AS_PTR(args[0])->pointer);
  RETURN_STRING(hostname);
}

DECLARE_MODULE_METHOD(ssl_get_accept_name) {
  ENFORCE_ARG_COUNT(get_accept_name, 1);
  ENFORCE_ARG_TYPE(get_accept_name, 0, IS_PTR);
  char *hostname = (char *)BIO_get_accept_name((BIO*)AS_PTR(args[0])->pointer);
  RETURN_STRING(hostname);
}

DECLARE_MODULE_METHOD(ssl_get_conn_address) {
  ENFORCE_ARG_COUNT(get_conn_address, 1);
  ENFORCE_ARG_TYPE(get_conn_address, 0, IS_PTR);
  char *address = (char *)BIO_get_conn_address((BIO*)AS_PTR(args[0])->pointer);
  RETURN_STRING(address);
}

DECLARE_MODULE_METHOD(ssl_get_conn_port) {
  ENFORCE_ARG_COUNT(get_conn_port, 1);
  ENFORCE_ARG_TYPE(get_conn_port, 0, IS_PTR);
  char *port = (char *)BIO_get_conn_port((BIO*)AS_PTR(args[0])->pointer);
  RETURN_STRING(port);
}

DECLARE_MODULE_METHOD(ssl_get_accept_port) {
  ENFORCE_ARG_COUNT(get_accept_port, 1);
  ENFORCE_ARG_TYPE(get_accept_port, 0, IS_PTR);
  char *port = (char *)BIO_get_accept_port((BIO*)AS_PTR(args[0])->pointer);
  RETURN_STRING(port);
}

DECLARE_MODULE_METHOD(ssl_get_conn_family) {
  ENFORCE_ARG_COUNT(get_conn_family, 1);
  ENFORCE_ARG_TYPE(get_conn_family, 0, IS_PTR);
  RETURN_NUMBER(BIO_get_conn_ip_family((BIO*)AS_PTR(args[0])->pointer));
}

DECLARE_MODULE_METHOD(ssl_get_accept_family) {
  ENFORCE_ARG_COUNT(get_accept_family, 1);
  ENFORCE_ARG_TYPE(get_accept_family, 0, IS_PTR);
  RETURN_NUMBER(BIO_get_accept_ip_family((BIO*)AS_PTR(args[0])->pointer));
}

DECLARE_MODULE_METHOD(ssl_set_nbio) {
  ENFORCE_ARG_COUNT(set_nbio, 2);
  ENFORCE_ARG_TYPE(set_nbio, 0, IS_PTR);
  ENFORCE_ARG_TYPE(set_nbio, 1, IS_BOOL);
  BIO_set_nbio((BIO*)AS_PTR(args[0])->pointer, AS_BOOL(args[1]) ? 1 : 0);
  RETURN;
}

DECLARE_MODULE_METHOD(ssl_bio_push) {
  ENFORCE_ARG_COUNT(push, 2);
  ENFORCE_ARG_TYPE(push, 0, IS_PTR); // input
  ENFORCE_ARG_TYPE(push, 1, IS_PTR); // output
  BIO *in = (BIO*)AS_PTR(args[0])->pointer;
  BIO *out = (BIO*)AS_PTR(args[1])->pointer;
  RETURN_PTR(BIO_push(in, out));
}

DECLARE_MODULE_METHOD(ssl_bio_pop) {
  ENFORCE_ARG_COUNT(push, 2);
  ENFORCE_ARG_TYPE(push, 0, IS_PTR); // input
  BIO *in = (BIO*)AS_PTR(args[0])->pointer;
  RETURN_PTR(BIO_pop(in));
}

DECLARE_MODULE_METHOD(ssl_write) {
  ENFORCE_ARG_COUNT(write, 2);
  ENFORCE_ARG_TYPE(write, 0, IS_PTR);
  ENFORCE_ARG_TYPE(write, 1, IS_STRING); // data

  BIO *bio = (BIO*)AS_PTR(args[0])->pointer;
  b_obj_string *string = AS_STRING(args[1]);
  char *p = string->chars;
  int len = string->length;

  int off = 0, total = 0;
  for (;;) {
    int i = BIO_write(bio, &(p[off]), len);
    total += i;
    if (i <= 0) {
      if (BIO_should_retry(bio)) {
//        sleep(1);
        continue;
      }
      RETURN_NUMBER(-1);
    }
    off += i;
    len -= i;
    if (len <= 0)
      break;
  }

  RETURN_NUMBER(total);
}

DECLARE_MODULE_METHOD(ssl_read) {
  ENFORCE_ARG_COUNT(read, 2);
  ENFORCE_ARG_TYPE(read, 0, IS_PTR);
  ENFORCE_ARG_TYPE(read, 1, IS_NUMBER);
  BIO *bio = (BIO*)AS_PTR(args[0])->pointer;
  int buffer_length = AS_NUMBER(args[1]);

  char *data = (char*)calloc(0, sizeof(char));
  int total = 0;

  for (;;) {
    char buf[buffer_length];
    int i = BIO_read(bio, buf, buffer_length);
    if (i == 0 || (i == -1 && total > 0))
      break;
    if (i < 0) {
      if (BIO_should_retry(bio)) {
        sleep(1);
        continue;
      }
      RETURN_NIL; // error...
    }

    data = (char*) realloc(data, sizeof(char) * (total + i + 1));
    memcpy(data + total, buf, i);
    total += i;
  }

  data[total] = '\0';
  RETURN_L_STRING(data, total);
}

DECLARE_MODULE_METHOD(ssl_should_retry) {
  ENFORCE_ARG_COUNT(should_retry, 1);
  ENFORCE_ARG_TYPE(should_retry, 0, IS_PTR);
  BIO *bio = (BIO*)AS_PTR(args[0])->pointer;
  RETURN_BOOL(BIO_should_retry(bio));
}

DECLARE_MODULE_METHOD(ssl_do_connect) {
  ENFORCE_ARG_COUNT(should_retry, 1);
  ENFORCE_ARG_TYPE(should_retry, 0, IS_PTR);
  BIO *bio = (BIO*)AS_PTR(args[0])->pointer;

  int result;
  for(;;) {
    result = BIO_do_connect(bio);
    if(result == 1) break;
    if (BIO_should_retry(bio)) {
      continue;
    }
    break;
  }

  RETURN_NUMBER(result);
}

DECLARE_MODULE_METHOD(ssl_error) {
  ENFORCE_ARG_COUNT(error, 1);
  ENFORCE_ARG_TYPE(error, 0, IS_PTR);
  SSL *ssl = (SSL*)AS_PTR(args[0])->pointer;
  RETURN_NUMBER(SSL_get_error(ssl, -1));
}

DECLARE_MODULE_METHOD(ssl_error_string) {
  ENFORCE_ARG_COUNT(error_string, 1);
  ENFORCE_ARG_TYPE(error_string, 0, IS_PTR);
  SSL *ssl = (SSL*)AS_PTR(args[0])->pointer;
  int code = SSL_get_error(ssl, -1);
  if(code != SSL_ERROR_SYSCALL) {
    char *err = ERR_error_string(code, NULL);
    RETURN_STRING(err);
  } else {
    char *error = strerror(errno);
    RETURN_STRING(error);
  }
}

DECLARE_MODULE_METHOD(ssl_free) {
  ENFORCE_ARG_COUNT(free, 1);
  ENFORCE_ARG_TYPE(free, 0, IS_PTR);
  BIO_free_all((BIO*)AS_PTR(args[0])->pointer);
  RETURN;
}


void __ssl_module_preloader(b_vm *vm) {
#ifdef WATT32
  dbug_init();
  sock_init();
#endif
}


CREATE_MODULE_LOADER(ssl) {

  static b_field_reg module_fields[] = {
      /**
       * constants
       */
      GET_SSL_CONSTANT(SSL_FILETYPE_PEM),
      GET_SSL_CONSTANT(SSL_FILETYPE_ASN1),

      /**
       * SSL Verify
       */
      GET_SSL_CONSTANT(SSL_VERIFY_NONE),
      GET_SSL_CONSTANT(SSL_VERIFY_PEER),
      GET_SSL_CONSTANT(SSL_VERIFY_FAIL_IF_NO_PEER_CERT),
      GET_SSL_CONSTANT(SSL_VERIFY_CLIENT_ONCE),
      GET_SSL_CONSTANT(SSL_VERIFY_POST_HANDSHAKE),

      /**
       * constant methods
       */
      GET_SSL_CONSTANT(TLS_method),
      GET_SSL_CONSTANT(TLS_client_method),
      GET_SSL_CONSTANT(TLS_server_method),
      GET_SSL_CONSTANT(SSLv23_method),
      GET_SSL_CONSTANT(SSLv23_client_method),
      GET_SSL_CONSTANT(SSLv23_server_method),

      /**
       * BIOs
       */
      GET_SSL_CONSTANT(BIO_CLOSE),
      GET_SSL_CONSTANT(BIO_NOCLOSE),
      GET_SSL_CONSTANT(BIO_f_ssl),
      GET_SSL_CONSTANT(BIO_s_connect),
      GET_SSL_CONSTANT(BIO_s_accept),

      {NULL,       false, NULL},
  };

  static b_func_reg module_functions[] = {
      {"ctx",   true,  GET_MODULE_METHOD(ssl_ctx)},
      {"ctx_free",   true,  GET_MODULE_METHOD(ssl_ctx_free)},
      {"ctx_set_verify",   true,  GET_MODULE_METHOD(ssl_ctx_set_verify)},
      {"ctx_set_verify_locations",   true,  GET_MODULE_METHOD(ssl_ctx_set_verify_locations)},
      {"new",   true,  GET_MODULE_METHOD(ssl_new)},
      {"ssl_free",   true,  GET_MODULE_METHOD(ssl_ssl_free)},
      {"set_connect_state",   true,  GET_MODULE_METHOD(ssl_set_connect_state)},
      {"new_bio",   true,  GET_MODULE_METHOD(ssl_new_bio)},
      {"set_ssl",   true,  GET_MODULE_METHOD(ssl_bio_set_ssl)},
      {"set_conn_hostname",   true,  GET_MODULE_METHOD(ssl_set_conn_hostname)},
      {"set_accept_name",   true,  GET_MODULE_METHOD(ssl_set_accept_name)},
      {"set_conn_address",   true,  GET_MODULE_METHOD(ssl_set_conn_address)},
      {"set_conn_port",   true,  GET_MODULE_METHOD(ssl_set_conn_port)},
      {"set_accept_port",   true,  GET_MODULE_METHOD(ssl_set_accept_port)},
      {"set_conn_family",   true,  GET_MODULE_METHOD(ssl_set_conn_family)},
      {"set_accept_family",   true,  GET_MODULE_METHOD(ssl_set_accept_family)},
      {"get_conn_hostname",   true,  GET_MODULE_METHOD(ssl_get_conn_hostname)},
      {"get_accept_name",   true,  GET_MODULE_METHOD(ssl_get_accept_name)},
      {"get_conn_address",   true,  GET_MODULE_METHOD(ssl_get_conn_address)},
      {"get_conn_port",   true,  GET_MODULE_METHOD(ssl_get_conn_port)},
      {"get_accept_port",   true,  GET_MODULE_METHOD(ssl_get_accept_port)},
      {"get_conn_family",   true,  GET_MODULE_METHOD(ssl_get_conn_family)},
      {"get_accept_family",   true,  GET_MODULE_METHOD(ssl_get_accept_family)},
      {"set_nbio",   true,  GET_MODULE_METHOD(ssl_set_nbio)},
      {"push",   true,  GET_MODULE_METHOD(ssl_bio_push)},
      {"pop",   true,  GET_MODULE_METHOD(ssl_bio_pop)},
      {"write",   true,  GET_MODULE_METHOD(ssl_write)},
      {"read",   true,  GET_MODULE_METHOD(ssl_read)},
      {"do_connect",   true,  GET_MODULE_METHOD(ssl_do_connect)},
      {"should_retry",   true,  GET_MODULE_METHOD(ssl_should_retry)},
      {"error",   true,  GET_MODULE_METHOD(ssl_error)},
      {"error_string",   true,  GET_MODULE_METHOD(ssl_error_string)},
      {"free",   true,  GET_MODULE_METHOD(ssl_free)},
      {NULL,    false, NULL},
  };

  static b_module_reg module = {
      .name = "_ssl",
      .fields = module_fields,
      .functions = module_functions,
      .classes = NULL,
      .preloader = &__ssl_module_preloader,
      .unloader = NULL
  };

  return &module;
}