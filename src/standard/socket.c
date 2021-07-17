#ifdef _MSC_VER
#pragma warning (disable : 5105)
#endif

#include "socket.h"
#include "blade_unistd.h"
#include "pathinfo.h"

#ifndef _WIN32
#include <sys/socket.h>
#include <arpa/inet.h>
#include <arpa/telnet.h>
//#include <arpa/ftp.h>
//#include <arpa/tftp.h>
//#include <arpa/nameser.h>
#include <netdb.h> //hostent
#include <sys/ioctl.h>
#endif

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

#ifdef _WIN32

#define _WINSOCK_DEPRECATED_NO_WARNINGS 1

#include <ws2tcpip.h>
#include "win32.h"
#include <winsock2.h>
#pragma comment (lib, "ws2_32") /* winsock support */
#include "blade_getopt.h"
#define sleep			_sleep
#define strcasecmp		strcmpi
#define ioctl ioctlsocket
#endif

#define BIGSIZ 8192		/* big buffers */
#define SMALLSIZ 256		/* small buffers, hostnames, etc */

DECLARE_MODULE_METHOD(socket__error) {
  ENFORCE_ARG_COUNT(_error, 1);
  ENFORCE_ARG_TYPE(_error, 0, IS_NUMBER);

  // do not report errno == EINPROGRESS, EWOULDBLOCK and EAGAIN as error
  if(AS_NUMBER(args[0]) == -1 && errno != EINPROGRESS && errno != EWOULDBLOCK && errno != EAGAIN) {
    char *error = strerror(errno);
    RETURN_STRING(error);
  }
  RETURN;
}

DECLARE_MODULE_METHOD(socket__create) {
  ENFORCE_ARG_COUNT(_create, 3);
  ENFORCE_ARG_TYPE(_create, 0, IS_NUMBER); // family
  ENFORCE_ARG_TYPE(_create, 1, IS_NUMBER); // type
  ENFORCE_ARG_TYPE(_create, 2, IS_NUMBER); // flags

#ifdef _WIN32
  WSADATA wsa_data;
  int i_result = WSAStartup(MAKEWORD(1, 1), &wsa_data);
  if (i_result != NO_ERROR) {
      errno = i_result;
      RETURN_NUMBER(-1);
  }
#endif

  RETURN_NUMBER(socket((int)AS_NUMBER(args[0]), (int)AS_NUMBER(args[1]), (int)AS_NUMBER(args[2])));
}

// @TODO: Support IPv6 connect...
DECLARE_MODULE_METHOD(socket__connect) {
  ENFORCE_ARG_COUNT(_connect, 6);
  ENFORCE_ARG_TYPE(_connect, 0, IS_NUMBER); // the socket id
  ENFORCE_ARG_TYPE(_connect, 1, IS_STRING); // the address
  ENFORCE_ARG_TYPE(_connect, 2, IS_NUMBER); // the port
  ENFORCE_ARG_TYPE(_connect, 3, IS_NUMBER); // the family
  ENFORCE_ARG_TYPE(_connect, 4, IS_NUMBER); // timeout
  ENFORCE_ARG_TYPE(_connect, 5, IS_BOOL); // is_blocking

  int sock = AS_NUMBER(args[0]);
  char *address = AS_C_STRING(args[1]);
  int port = AS_NUMBER(args[2]);
  int family = AS_NUMBER(args[3]);
  int time_out = AS_NUMBER(args[4]);
  bool is_blocking = AS_BOOL(args[5]);

  struct sockaddr_in remote = {0};

  remote.sin_addr.s_addr = inet_addr(address);
  remote.sin_family = family;
  remote.sin_port = htons(port);

  if(inet_pton(family, address, &remote.sin_addr) <= 0) {
    errno = EADDRNOTAVAIL;
    RETURN_NUMBER(-1);
  }

  fd_set read_set;
  FD_ZERO(&read_set);
  if(!FD_ISSET(sock, &read_set)) {
    FD_SET(sock, &read_set);//tcp socket
  }

#ifndef _WIN32
  long arg = fcntl(sock, F_GETFL) | O_NONBLOCK;
  bool non_blocking = fcntl(sock, F_SETFL, arg) == 0;
#else
  unsigned long arg = 1;
  bool non_blocking = ioctl(sock, FIONBIO, &arg) == 0;
#endif

  if (connect(sock, (struct sockaddr*) & remote, sizeof(remote)) < 0) {
#ifndef _WIN32
    if (errno != EINPROGRESS) {
#else
    if (errno != ENOENT && errno != EINPROGRESS) {
#endif // !_WIN32
      RETURN_NUMBER(-1);
    }
  }

  // getting the timeout...
  struct timeval timeout = {(long)(time_out / 1000), (int)((time_out % 1000) * 1000)};

  if(select(sock + 1, NULL, &read_set, NULL, &timeout)) {
    int so_error;
    socklen_t len = sizeof so_error;

#ifndef _WIN32
    getsockopt(sock, SOL_SOCKET, SO_ERROR, &so_error, &len);
#else
    getsockopt(sock, SOL_SOCKET, SO_ERROR, (char *)&so_error, &len);
#endif
    if (so_error == 0) {
      if(is_blocking && non_blocking) {
#ifndef _WIN32
        arg &= (~O_NONBLOCK);
        fcntl(sock, F_SETFL, arg);
#else
        unsigned long arg = 0;
        ioctl(sock, FIONBIO, &arg);
#endif
      }
      RETURN_NUMBER(so_error);
    } else {
      errno = so_error;
    }
  } else {
    errno = ETIMEDOUT;
  }
  RETURN_NUMBER(-1);
}

// @TODO: Support IPv6 bind...
DECLARE_MODULE_METHOD(socket__bind) {
  ENFORCE_ARG_COUNT(_bind, 4);
  ENFORCE_ARG_TYPE(_bind, 0, IS_NUMBER); // the socket id
  ENFORCE_ARG_TYPE(_bind, 1, IS_STRING); // the address
  ENFORCE_ARG_TYPE(_bind, 2, IS_NUMBER); // the port
  ENFORCE_ARG_TYPE(_bind, 3, IS_NUMBER); // the family

  int sock = AS_NUMBER(args[0]);
  char *address = AS_C_STRING(args[1]);
  int port = AS_NUMBER(args[2]);
  int family = AS_NUMBER(args[3]);

  struct sockaddr_in remote = {0};

  remote.sin_addr.s_addr = inet_addr(address);
  remote.sin_family = family;
  remote.sin_port = htons(port);

  if(inet_pton(AF_INET, address, &remote.sin_addr) <= 0) {
    RETURN_ERROR("address not valid or unsupported");
  }

  RETURN_NUMBER(bind(sock, (struct sockaddr *)&remote, sizeof(remote)));
}

DECLARE_MODULE_METHOD(socket__listen) {
  ENFORCE_ARG_COUNT(_bind, 2);
  ENFORCE_ARG_TYPE(_bind, 0, IS_NUMBER); // the socket id
  ENFORCE_ARG_TYPE(_bind, 1, IS_NUMBER); // backlog

  int sock = AS_NUMBER(args[0]);
  int backlog = AS_NUMBER(args[1]);

  RETURN_NUMBER(listen(sock, backlog));
}

DECLARE_MODULE_METHOD(socket__accept) {
  ENFORCE_ARG_COUNT(_accept, 1);
  ENFORCE_ARG_TYPE(_bind, 0, IS_NUMBER); // the socket id

  int sock = AS_NUMBER(args[0]);

  struct sockaddr_in client = {0};
  int client_length = sizeof(struct sockaddr_in);
  int new_sock = accept(sock, (struct sockaddr*)&client, (socklen_t*)&client_length);
  if(new_sock < 0) {
    RETURN_ERROR("client accept failed");
  }

  char *ip = inet_ntoa(client.sin_addr);
  int port = (int) ntohs(client.sin_port);

  b_obj_list *response = new_list(vm);
  write_list(vm, response, NUMBER_VAL(new_sock));
  write_list(vm, response, OBJ_VAL(take_string(vm, ip, (int)strlen(ip))));
  write_list(vm, response, NUMBER_VAL(port));

  RETURN_OBJ(response);
}

DECLARE_MODULE_METHOD(socket__send) {
  ENFORCE_ARG_COUNT(_send, 3);
  ENFORCE_ARG_TYPE(_send, 0, IS_NUMBER); // the socket id
  ENFORCE_ARG_TYPE(_send, 2, IS_NUMBER); // flags

  int sock = AS_NUMBER(args[0]);
  b_value data = args[1];
  int flags = AS_NUMBER(args[2]);

  char *content = NULL;
  int length;

  if(IS_STRING(data)) {
    content = AS_STRING(data)->chars;
    length = AS_STRING(data)->length;
  } else if(IS_FILE(data)) {
    content = read_file(realpath(AS_FILE(data)->path->chars, NULL));
    length = (int)strlen(content);
  } else {
    content = value_to_string(vm, data);
    length = (int)strlen(content);
  }

  RETURN_NUMBER(send(sock, content, length, flags));
}

DECLARE_MODULE_METHOD(socket__recv) {
  ENFORCE_ARG_COUNT(_recv, 3);
  ENFORCE_ARG_TYPE(_recv, 0, IS_NUMBER); // the socket id
  ENFORCE_ARG_TYPE(_recv, 1, IS_NUMBER); // length to read
  ENFORCE_ARG_TYPE(_recv, 2, IS_NUMBER); // flags

  int sock = AS_NUMBER(args[0]);
  int length = AS_NUMBER(args[1]);
  int flags = AS_NUMBER(args[2]);

  struct timeval timeout;
  int option_length = 0;

#ifndef _WIN32
  int rc = getsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, (socklen_t*)&option_length);
#else
  int rc = getsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, (socklen_t*)&option_length);
#endif // !_WIN32

  if(rc != 0 || option_length != sizeof timeout || (timeout.tv_sec == 0 && timeout.tv_usec == 0)) {
    // set default timeout to 5 minutes
    timeout.tv_sec = 300;
    timeout.tv_usec = 0;
  }

  fd_set read_set;
  FD_ZERO(&read_set);
  if(!FD_ISSET(STDIN_FILENO, &read_set)) {
    FD_SET(STDIN_FILENO, &read_set);//tcp socket
  }
  if(!FD_ISSET(sock, &read_set)) {
    FD_SET(sock, &read_set);//tcp socket
  }

  if(select(sock + 1, &read_set, NULL, NULL, &timeout)) {
    int content_length;
    ioctl(sock, FIONREAD, &content_length);

    if(content_length > 0) {
      if(length != -1 && length < content_length)
        content_length = length;

      char *response = (char*)ALLOCATE(char, (size_t)content_length + 1);
      ssize_t total_length = recv(sock, response, content_length, flags);
      response[total_length] = '\0';

      RETURN_T_STRING(response, total_length);
    }
  } else {
    errno = ETIMEDOUT;
    RETURN_NUMBER(-1);
  }
  RETURN;
}

DECLARE_MODULE_METHOD(socket__setsockopt) {
  ENFORCE_ARG_COUNT(_setsockopt, 3);
  ENFORCE_ARG_TYPE(_setsockopt, 0, IS_NUMBER); // the socket id
  ENFORCE_ARG_TYPE(_setsockopt, 1, IS_NUMBER); // the option id

  int sock = AS_NUMBER(args[0]);
  int option = AS_NUMBER(args[1]);
  b_value value = args[2];

  switch(option) {
    case SO_SNDTIMEO:
    case SO_RCVTIMEO: {
      ENFORCE_ARG_TYPE(_setsockopt, 2, IS_NUMBER);

#ifdef _WIN32
      DWORD timeout = AS_NUMBER(value);
      RETURN_NUMBER(setsockopt(sock, SOL_SOCKET, option, (const char*)&timeout, sizeof(timeout)));
#else
      int milliseconds = AS_NUMBER(value);
      struct timeval tv = {(long)(milliseconds /  1000),
                           (int)((milliseconds % 1000) * 1000)};

      RETURN_NUMBER(setsockopt(sock, SOL_SOCKET, option, (const char*)&tv, sizeof tv));
#endif
    }
    default: {
      ENFORCE_ARG_TYPE(_setsockopt, 2, IS_BOOL);
      int val = AS_BOOL(value) ? 1 : 0;
      RETURN_NUMBER(setsockopt(sock, SOL_SOCKET, option, (const char*)&val, sizeof val));
    }
  }
}

DECLARE_MODULE_METHOD(socket__getsockopt) {
  ENFORCE_ARG_COUNT(_getsockopt, 2);
  ENFORCE_ARG_TYPE(_getsockopt, 0, IS_NUMBER); // the socket id
  ENFORCE_ARG_TYPE(_getsockopt, 1, IS_NUMBER); // the option id

  int sock = AS_NUMBER(args[0]);
  int option = AS_NUMBER(args[1]);

  switch(option) {
    case SO_ERROR: {
      int so_error;
      socklen_t len = sizeof so_error;

#ifndef _WIN32
      getsockopt(sock, SOL_SOCKET, SO_ERROR, &so_error, &len);
#else
      getsockopt(sock, SOL_SOCKET, SO_ERROR, (char *)&so_error, &len);
#endif // !_WIN32

      if(so_error == 0) RETURN;
      char * error = strerror(so_error);
      RETURN_STRING(error);
    }

    case SO_SNDTIMEO:
    case SO_RCVTIMEO: {

#ifdef _WIN32
      DWORD timeout;
      int len = sizeof(timeout);
      if(getsockopt(sock, SOL_SOCKET, option, (char *)&timeout, &len) >= 0) {
        RETURN_NUMBER(timeout);
      }
#else
      struct timeval tv;
      socklen_t len = sizeof tv;
      getsockopt(sock, SOL_SOCKET, option, &tv, &len);
      if(len == sizeof tv) {
        RETURN_NUMBER((tv.tv_sec * 1000) + ((double)tv.tv_usec / 1000));
      }
#endif
      RETURN_NUMBER(-1);
    }
    default: {
      int so_result;
      socklen_t len = sizeof so_result;
#ifndef _WIN32
      getsockopt(sock, SOL_SOCKET, option, &so_result, &len);
#else
      getsockopt(sock, SOL_SOCKET, option, (char *)&so_result, &len);
#endif // _WIN32

      if(len == sizeof so_result) {
        RETURN_NUMBER(so_result);
      }
      RETURN_NUMBER(-1);
    }
  }
}

// @TODO: Add IPv6 support...
DECLARE_MODULE_METHOD(socket__getsockinfo) {
  ENFORCE_ARG_COUNT(_getsockinfo, 1);
  ENFORCE_ARG_TYPE(_getsockinfo, 0, IS_NUMBER);

  int sock = AS_NUMBER(args[0]);

  struct sockaddr_in address;
  memset(&address, 0, sizeof(address));

  int length = sizeof address;
  if(getsockname(sock, (struct sockaddr *)&address, (socklen_t*)&length) >= 0) {
    char *ip = inet_ntoa(address.sin_addr);
    int port = ntohs(address.sin_port);

    b_obj_dict *dict = (b_obj_dict *)GC(new_dict(vm));

    dict_add_entry(vm, dict, GC_L_STRING("address", 7), GC_STRING(ip));
    dict_add_entry(vm, dict, GC_L_STRING("port", 4), NUMBER_VAL(port));
    dict_add_entry(vm, dict, GC_L_STRING("family", 6),
                   NUMBER_VAL(ntohs(address.sin_family)));

    RETURN_OBJ(dict);
  }

  RETURN_NUMBER(-1);
}

// @TODO: Add IPv6 support...
DECLARE_MODULE_METHOD(socket__getaddrinfo) {
  ENFORCE_ARG_COUNT(_getaddrinfo, 3);
  ENFORCE_ARG_TYPE(_getaddrinfo, 0, IS_STRING);
  ENFORCE_ARG_TYPE(_getaddrinfo, 2, IS_NUMBER);

  b_obj_string *addr = AS_STRING(args[0]);
  char *type = "http";
  if(!IS_NIL(args[1])){
    ENFORCE_ARG_TYPE(_getaddrinfo, 1, IS_STRING);
    type = AS_C_STRING(args[1]);
  }
  int family = AS_NUMBER(args[2]);

  struct addrinfo *res, hints = {0};

  hints.ai_socktype = SOCK_STREAM;
  hints.ai_family = family;

#ifdef _WIN32
  WSADATA wsa_data;
  int i_result = WSAStartup(MAKEWORD(1, 1), &wsa_data);
  if (i_result != NO_ERROR) {
    RETURN;
  }
#endif

  if(getaddrinfo(addr->length > 0 ? addr->chars : NULL, type,  &hints, &res) == 0) {
    while(res) {
      if(res->ai_family == family) {

        b_obj_dict *dict = (b_obj_dict*)GC(new_dict(vm));
        if(res->ai_canonname != NULL) {
          dict_add_entry(vm, dict, GC_L_STRING("cannon_name", 11), GC_STRING(res->ai_canonname));
        } else {
          dict_add_entry(vm, dict, GC_L_STRING("cannon_name", 11), NIL_VAL);
        }

        char *result = NULL;

        switch(family) {
          case AF_INET: {
            void *ptr = &((struct sockaddr_in*) res->ai_addr)->sin_addr;
            result = ALLOCATE(char, 16); // INET_ADDRSTRLEN
            inet_ntop(res->ai_family, ptr, result, 16);
            break;
          }
          case AF_INET6: {
            void *ptr = &((struct sockaddr_in6*) res->ai_addr)->sin6_addr;
            result = ALLOCATE(char, 46); // INET6_ADDRSTRLEN
            inet_ntop(res->ai_family, ptr, result, 46);
            break;
          }
          default: {
            result = ALLOCATE(char, 1);
            result[0] = '\0';
            break;
          }
        }

        dict_add_entry(vm, dict, GC_L_STRING("ip", 2), GC_TT_STRING(result));
        RETURN_OBJ(dict);
      }
    }
  }

  RETURN;
}

DECLARE_MODULE_METHOD(socket__close) {
  ENFORCE_ARG_COUNT(_error, 1);
  ENFORCE_ARG_TYPE(_error, 0, IS_NUMBER);
  int result = closesocket((int)AS_NUMBER(args[0]));
  RETURN_NUMBER(result);
}

DECLARE_MODULE_METHOD(socket__shutdown) {
  ENFORCE_ARG_COUNT(_error, 2);
  ENFORCE_ARG_TYPE(_error, 0, IS_NUMBER);
  ENFORCE_ARG_TYPE(_error, 0, IS_NUMBER);
  RETURN_NUMBER(shutdown((int) AS_NUMBER(args[0]), (int) AS_NUMBER(args[1])));
}

CREATE_MODULE_LOADER(socket) {

  static b_func_reg http_class_functions[] = {
      {"_create", false, GET_MODULE_METHOD(socket__create)},
      {"_connect", false, GET_MODULE_METHOD(socket__connect)},
      {"_send", false, GET_MODULE_METHOD(socket__send)},
      {"_recv", false, GET_MODULE_METHOD(socket__recv)},
      {"_setsockopt", false, GET_MODULE_METHOD(socket__setsockopt)},
      {"_getsockopt", false, GET_MODULE_METHOD(socket__getsockopt)},
      {"_bind", false, GET_MODULE_METHOD(socket__bind)},
      {"_listen", false, GET_MODULE_METHOD(socket__listen)},
      {"_accept", false, GET_MODULE_METHOD(socket__accept)},
      {"_error", false, GET_MODULE_METHOD(socket__error)},
      {"_close", false, GET_MODULE_METHOD(socket__close)},
      {"_shutdown", false, GET_MODULE_METHOD(socket__shutdown)},
      {"_getsockinfo", false, GET_MODULE_METHOD(socket__getsockinfo)},
      {"_getaddrinfo", false, GET_MODULE_METHOD(socket__getaddrinfo)},
      {NULL, false, NULL},
  };

  static b_field_reg http_class_fields[] = {
      {NULL, false, NULL},
  };

  static b_class_reg classes[] = {
      {"Socket", http_class_fields, http_class_functions},
      {NULL, NULL, NULL},
  };

  static b_module_reg module = {NULL,NULL, classes};

  // @TODO: WSACleanup() on module cleanup function when implemented.

  return module;
}

#undef BIGSIZ
#undef SMALLSIZ