#ifdef _MSC_VER
# pragma warning (disable : 5105)
#endif

#include "module.h"
#include "pathinfo.h"

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

#ifdef HAVE_UNISTD_H
# include <unistd.h>
#else
# include "blade_unistd.h"
#endif /* HAVE_UNISTD_H */

#ifdef _WIN32
# define _WINSOCK_DEPRECATED_NO_WARNINGS 1
# include <sdkddkver.h>
# include <ws2tcpip.h>
# include <winsock2.h>
# include <blade_unistd.h>

# define sleep			_sleep
# ifndef strcasecmp
#   define strcasecmp		strcmpi
# endif
# define ioctl ioctlsocket
# ifndef STDIN_FILENO
#   define STDIN_FILENO _fileno(stdin)
# endif
#else
# include <sys/socket.h>
# include <arpa/inet.h>
# include <netdb.h> //hostent
# include <sys/ioctl.h>
# define closesocket close
#endif

#define BIGSIZ 8192    /* big buffers */
#define SMALLSIZ 256    /* small buffers, hostnames, etc */

DECLARE_MODULE_METHOD(socket__error) {
  ENFORCE_ARG_COUNT(_error, 1);
  ENFORCE_ARG_TYPE(_error, 0, IS_NUMBER);

  // do not report errno == EINPROGRESS, EWOULDBLOCK and EAGAIN as error
  if (AS_NUMBER(args[0]) == -1 && errno != EINPROGRESS && errno != EWOULDBLOCK && errno != EAGAIN) {
    char *error = strerror(errno);
    RETURN_STRING(error);
  }
  RETURN_NIL;
}

DECLARE_MODULE_METHOD(socket__create) {
  ENFORCE_ARG_COUNT(_create, 3);
  ENFORCE_ARG_TYPE(_create, 0, IS_NUMBER); // family
  ENFORCE_ARG_TYPE(_create, 1, IS_NUMBER); // type
  ENFORCE_ARG_TYPE(_create, 2, IS_NUMBER); // protocol

  int family = (int) AS_NUMBER(args[0]);
  int type = (int) AS_NUMBER(args[1]);

#ifdef SOCK_CLOEXEC
  type |= SOCK_CLOEXEC;
#endif

  int protocol = (int) AS_NUMBER(args[2]);

  int sock;
  if ((sock = socket(family, type, protocol)) < 0) {
    RETURN_NUMBER(-1);
  }

#ifndef _WIN32
# ifdef SO_NOSIGPIPE
  int set = 1;
  if (setsockopt(sock, SOL_SOCKET, SO_NOSIGPIPE, &set, sizeof(int)) < 0) {
    // do nothing. this is just an optimization.
  }
# endif
#endif

  RETURN_NUMBER(sock);
}

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

  if (inet_pton(family, address, &remote.sin_addr) <= 0) {
    errno = EADDRNOTAVAIL;
    RETURN_NUMBER(-1);
  }

  fd_set read_set;
  FD_ZERO(&read_set);
  if (!FD_ISSET(sock, &read_set)) {
    FD_SET(sock, &read_set);//tcp socket
  }

#ifndef _WIN32
  long arg = fcntl(sock, F_GETFL) | O_NONBLOCK;
  bool non_blocking = fcntl(sock, F_SETFL, arg) == 0;
#else
  unsigned long arg = 1;
  bool non_blocking = ioctl(sock, FIONBIO, &arg) == 0;
#endif

  int con_result = -1;

#if !defined(_WIN32) && defined(EINTR)
  for (;;) {
    con_result = connect(sock, (struct sockaddr *) &remote, sizeof(remote));
    if (con_result >= 0 || errno != EINTR)
      break;
  }
#else
  con_result = connect(sock, (struct sockaddr*) & remote, sizeof(remote));
#endif

  if (con_result < 0) {
#ifndef _WIN32
    if (errno != EINPROGRESS) {
#else
      if (errno != ENOENT && errno != EINPROGRESS) {
#endif // !_WIN32
      RETURN_NUMBER(-1);
    }
  }

  // getting the timeout...
  struct timeval timeout = {(long) (time_out / 1000), (int) ((time_out % 1000) * 1000)};

  if (select(sock + 1, NULL, &read_set, NULL, &timeout)) {
    int so_error;
    socklen_t len = sizeof so_error;

#ifndef _WIN32
    getsockopt(sock, SOL_SOCKET, SO_ERROR, &so_error, &len);
#else
    getsockopt(sock, SOL_SOCKET, SO_ERROR, (char *)&so_error, &len);
#endif
    if (so_error == 0) {
      if (is_blocking && non_blocking) {
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

  if (inet_pton(AF_INET, address, &remote.sin_addr) <= 0) {
    RETURN_ERROR("address not valid or unsupported");
  }

  RETURN_NUMBER(bind(sock, (struct sockaddr *) &remote, sizeof(remote)));
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
  ENFORCE_ARG_TYPE(_accept, 0, IS_NUMBER); // the socket id

  int sock = AS_NUMBER(args[0]);

  struct sockaddr_in client = {0};
  int client_length = sizeof(struct sockaddr_in);
  int new_sock = accept(sock, (struct sockaddr *) &client, (socklen_t *) &client_length);
  if (new_sock < 0) {
    RETURN_ERROR("client accept failed");
  }

  char *ip = inet_ntoa(client.sin_addr);
  int port = (int) ntohs(client.sin_port);

  b_obj_list *response = new_list(vm);
  write_list(vm, response, NUMBER_VAL(new_sock));
  write_list(vm, response, OBJ_VAL(copy_string(vm, ip, (int) strlen(ip))));
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

  if (IS_STRING(data)) {
    content = AS_STRING(data)->chars;
    length = AS_STRING(data)->length;
  } else if (IS_FILE(data)) {
    content = read_file(realpath(AS_FILE(data)->path->chars, NULL));
    length = (int) strlen(content);
  } else {
    content = value_to_string(vm, data);
    length = (int) strlen(content);
  }

#ifdef __linux__
#ifdef MSG_NOSIGNAL
  flags |= MSG_NOSIGNAL;
#endif
#endif

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
  int option_length = sizeof(timeout);

#ifndef _WIN32
  int rc = getsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, (socklen_t *) &option_length);
#else
  int rc = getsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, (socklen_t*)&option_length);
#endif // !_WIN32

  if (rc != 0 || sizeof(timeout) != option_length || (timeout.tv_sec == 0 && timeout.tv_usec == 0)) {
    // set default timeout to 5 minutes
    timeout.tv_sec = 300;
    timeout.tv_usec = 0;
  }

  fd_set read_set;
  FD_ZERO(&read_set);
  if (!FD_ISSET(sock, &read_set)) {
    FD_SET(sock, &read_set);//tcp socket
  }

  int status;
  if ((status = select(sock + 1, &read_set, NULL, NULL, &timeout)) > 0) {
    int content_length;
    ioctl(sock, FIONREAD, &content_length);

    if (content_length > 0) {
      if (length != -1 && length < content_length)
        content_length = length;

      char *response = (char *) ALLOCATE(char, (size_t) content_length + 1);
      ssize_t total_length = recv(sock, response, content_length, flags);
      response[total_length] = '\0';

      RETURN_T_STRING(response, total_length);
    }
  } else if (status == 0) {
    errno = ETIMEDOUT;
    RETURN_NUMBER(-1);
  }

  RETURN_NIL;
}

DECLARE_MODULE_METHOD(socket__setsockopt) {
  ENFORCE_ARG_COUNT(_setsockopt, 3);
  ENFORCE_ARG_TYPE(_setsockopt, 0, IS_NUMBER); // the socket id
  ENFORCE_ARG_TYPE(_setsockopt, 1, IS_NUMBER); // the option id

  int sock = AS_NUMBER(args[0]);
  int option = AS_NUMBER(args[1]);
  b_value value = args[2];

  switch (option) {
    case SO_SNDTIMEO:
    case SO_RCVTIMEO: {
      ENFORCE_ARG_TYPE(_setsockopt, 2, IS_NUMBER);

#ifdef _WIN32
      DWORD timeout = AS_NUMBER(value);
      RETURN_NUMBER(setsockopt(sock, SOL_SOCKET, option, (const char*)&timeout, sizeof(timeout)));
#else
      int milliseconds = AS_NUMBER(value);
      struct timeval tv = {(long) (milliseconds / 1000),
                           (int) ((milliseconds % 1000) * 1000)};

      RETURN_NUMBER(setsockopt(sock, SOL_SOCKET, option, &tv, sizeof(tv)));
#endif
    }
    default: {
      ENFORCE_ARG_TYPE(_setsockopt, 2, IS_BOOL);
      int val = AS_BOOL(value) ? 1 : 0;
      RETURN_NUMBER(setsockopt(sock, SOL_SOCKET, option, (const char *) &val, sizeof val));
    }
  }
}

DECLARE_MODULE_METHOD(socket__getsockopt) {
  ENFORCE_ARG_COUNT(_getsockopt, 2);
  ENFORCE_ARG_TYPE(_getsockopt, 0, IS_NUMBER); // the socket id
  ENFORCE_ARG_TYPE(_getsockopt, 1, IS_NUMBER); // the option id

  int sock = AS_NUMBER(args[0]);
  int option = AS_NUMBER(args[1]);

  switch (option) {
    case SO_ERROR: {
      int so_error;
      socklen_t len = sizeof so_error;

#ifndef _WIN32
      getsockopt(sock, SOL_SOCKET, SO_ERROR, &so_error, &len);
#else
      getsockopt(sock, SOL_SOCKET, SO_ERROR, (char *)&so_error, &len);
#endif // !_WIN32

      if (so_error == 0) RETURN_NIL;
      char *error = strerror(so_error);
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
      if (len == sizeof tv) {
        RETURN_NUMBER((tv.tv_sec * 1000) + ((double) tv.tv_usec / 1000));
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

      if (len == sizeof so_result) {
        RETURN_NUMBER(so_result);
      }
      RETURN_NUMBER(-1);
    }
  }
}

DECLARE_MODULE_METHOD(socket__getsockinfo) {
  ENFORCE_ARG_COUNT(_getsockinfo, 1);
  ENFORCE_ARG_TYPE(_getsockinfo, 0, IS_NUMBER);

  int sock = AS_NUMBER(args[0]);

  struct sockaddr_in address;
  memset(&address, 0, sizeof(address));

  int length = sizeof address;
  if (getsockname(sock, (struct sockaddr *) &address, (socklen_t *) &length) >= 0) {
    char *ip = inet_ntoa(address.sin_addr);
    int port = ntohs(address.sin_port);

    b_obj_dict *dict = (b_obj_dict *) GC(new_dict(vm));

    dict_add_entry(vm, dict, GC_L_STRING("address", 7), GC_STRING(ip));
    dict_add_entry(vm, dict, GC_L_STRING("port", 4), NUMBER_VAL(port));
    dict_add_entry(vm, dict, GC_L_STRING("family", 6),
                   NUMBER_VAL(ntohs(address.sin_family)));

    RETURN_OBJ(dict);
  }

  RETURN_NUMBER(-1);
}

DECLARE_MODULE_METHOD(socket__getaddrinfo) {
  ENFORCE_ARG_COUNT(_getaddrinfo, 3);
  ENFORCE_ARG_TYPE(_getaddrinfo, 0, IS_STRING);
  ENFORCE_ARG_TYPE(_getaddrinfo, 2, IS_NUMBER);

  b_obj_string *addr = AS_STRING(args[0]);
  char *type = "80";
  if (!IS_NIL(args[1])) {
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
    RETURN_NIL;
  }
#endif

  if (getaddrinfo(addr->length > 0 ? addr->chars : NULL, type, &hints, &res) == 0) {
    while (res) {
      if (res->ai_family == family) {

        b_obj_dict *dict = (b_obj_dict *) GC(new_dict(vm));
        if (res->ai_canonname != NULL) {
          dict_add_entry(vm, dict, GC_L_STRING("cannon_name", 11), GC_STRING(res->ai_canonname));
        } else {
          dict_add_entry(vm, dict, GC_L_STRING("cannon_name", 11), NIL_VAL);
        }

        char *result = NULL;

        switch (family) {
          case AF_INET: {
            void *ptr = &((struct sockaddr_in *) res->ai_addr)->sin_addr;
            result = ALLOCATE(char, 17); // INET_ADDRSTRLEN
            inet_ntop(res->ai_family, ptr, result, 16);
            result[16] = '\0';
            break;
          }
          case AF_INET6: {
            void *ptr = &((struct sockaddr_in6 *) res->ai_addr)->sin6_addr;
            result = ALLOCATE(char, 47); // INET6_ADDRSTRLEN
            inet_ntop(res->ai_family, ptr, result, 46);
            result[46] = '\0';
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

  RETURN_NIL;
}

DECLARE_MODULE_METHOD(socket__close) {
  ENFORCE_ARG_COUNT(_close, 1);
  ENFORCE_ARG_TYPE(_close, 0, IS_NUMBER);
  int sock = AS_NUMBER(args[0]);

//  // discard all leftover readable data...
//  char buf[16];
//  while (read(sock, buf, sizeof(buf)-1) > 0){}

  // close socket
  RETURN_NUMBER(closesocket(sock));

}

DECLARE_MODULE_METHOD(socket__shutdown) {
  ENFORCE_ARG_COUNT(_shutdown, 2);
  ENFORCE_ARG_TYPE(_shutdown, 0, IS_NUMBER);
  ENFORCE_ARG_TYPE(_shutdown, 0, IS_NUMBER);
  RETURN_NUMBER(shutdown((int) AS_NUMBER(args[0]), (int) AS_NUMBER(args[1])));
}

void __socket_module_unload(b_vm *vm) {
#ifdef _WIN32
  WSACleanup();
#endif
}

void __socket_module_preloader(b_vm *vm) {
#ifdef _WIN32
  WSADATA wsa_data;
  int i_result = WSAStartup(MAKEWORD(2, 2), &wsa_data);
  if (i_result != NO_ERROR) {
    errno = i_result;
    return;
  }

  if(LOBYTE(wsa_data.wVersion) != 2 || HIBYTE(wsa_data.wVersion) != 2) {
    WSACleanup();
    return;
  }
#else
#  ifdef SIGPIPE
  signal(SIGPIPE, SIG_IGN);
#  endif
#endif
}


/** START SOCKET CONSTANTS */

//  stream socket
b_value __socket_SOCK_STREAM(b_vm *vm) {
#ifdef SOCK_STREAM
  return NUMBER_VAL(SOCK_STREAM);
#else
  return NUMBER_VAL(-1);
#endif
}

//  datagram socket
b_value __socket_SOCK_DGRAM(b_vm *vm) {
#ifdef SOCK_DGRAM
  return NUMBER_VAL(SOCK_DGRAM);
#else
  return NUMBER_VAL(-1);
#endif
}

//  raw-protocol interface
b_value __socket_SOCK_RAW(b_vm *vm) {
#ifdef SOCK_RAW
  return NUMBER_VAL(SOCK_RAW);
#else
  return NUMBER_VAL(-1);
#endif
}

//  reliably-delivered message
b_value __socket_SOCK_RDM(b_vm *vm) {
#ifdef SOCK_RDM
  return NUMBER_VAL(SOCK_RDM);
#else
  return NUMBER_VAL(-1);
#endif
}

//  sequenced packet stream
b_value __socket_SOCK_SEQPACKET(b_vm *vm) {
#ifdef SOCK_SEQPACKET
  return NUMBER_VAL(SOCK_SEQPACKET);
#else
  return NUMBER_VAL(-1);
#endif
}


//  turn on debugging info recording
b_value __socket_SO_DEBUG(b_vm *vm) {
#ifdef SO_DEBUG
  return NUMBER_VAL(SO_DEBUG);
#else
  return NUMBER_VAL(-1);
#endif
}

//  socket has had listen()
b_value __socket_SO_ACCEPTCONN(b_vm *vm) {
#ifdef SO_ACCEPTCONN
  return NUMBER_VAL(SO_ACCEPTCONN);
#else
  return NUMBER_VAL(-1);
#endif
}

//  allow local address reuse
b_value __socket_SO_REUSEADDR(b_vm *vm) {
#ifdef SO_REUSEADDR
  return NUMBER_VAL(SO_REUSEADDR);
#else
  return NUMBER_VAL(-1);
#endif
}

//  keep connections alive
b_value __socket_SO_KEEPALIVE(b_vm *vm) {
#ifdef SO_KEEPALIVE
  return NUMBER_VAL(SO_KEEPALIVE);
#else
  return NUMBER_VAL(-1);
#endif
}

//  just use interface addresses
b_value __socket_SO_DONTROUTE(b_vm *vm) {
#ifdef SO_DONTROUTE
  return NUMBER_VAL(SO_DONTROUTE);
#else
  return NUMBER_VAL(-1);
#endif
}

//  permit sending of broadcast msgs
b_value __socket_SO_BROADCAST(b_vm *vm) {
#ifdef SO_BROADCAST
  return NUMBER_VAL(SO_BROADCAST);
#else
  return NUMBER_VAL(-1);
#endif
}

//  bypass hardware when possible
b_value __socket_SO_USELOOPBACK(b_vm *vm) {
#ifdef SO_USELOOPBACK
  return NUMBER_VAL(SO_USELOOPBACK);
#else
  return NUMBER_VAL(-1);
#endif
}

//  linger on close if data present (in ticks)
b_value __socket_SO_LINGER(b_vm *vm) {
#ifdef SO_LINGER
  return NUMBER_VAL(SO_LINGER);
#else
  return NUMBER_VAL(-1);
#endif
}

//  leave received OOB data in line
b_value __socket_SO_OOBINLINE(b_vm *vm) {
#ifdef SO_OOBINLINE
  return NUMBER_VAL(SO_OOBINLINE);
#else
  return NUMBER_VAL(-1);
#endif
}

//  allow local address & port reuse
b_value __socket_SO_REUSEPORT(b_vm *vm) {
#ifdef SO_REUSEPORT
  return NUMBER_VAL(SO_REUSEPORT);
#else
  return NUMBER_VAL(-1);
#endif
}

//  timestamp received dgram traffic
b_value __socket_SO_TIMESTAMP(b_vm *vm) {
#ifdef SO_TIMESTAMP
  return NUMBER_VAL(SO_TIMESTAMP);
#else
  return NUMBER_VAL(-1);
#endif
}


//  send buffer size
b_value __socket_SO_SNDBUF(b_vm *vm) {
#ifdef SO_SNDBUF
  return NUMBER_VAL(SO_SNDBUF);
#else
  return NUMBER_VAL(-1);
#endif
}

//  receive buffer size
b_value __socket_SO_RCVBUF(b_vm *vm) {
#ifdef SO_RCVBUF
  return NUMBER_VAL(SO_RCVBUF);
#else
  return NUMBER_VAL(-1);
#endif
}

//  send low-water mark
b_value __socket_SO_SNDLOWAT(b_vm *vm) {
#ifdef SO_SNDLOWAT
  return NUMBER_VAL(SO_SNDLOWAT);
#else
  return NUMBER_VAL(-1);
#endif
}

//  receive low-water mark
b_value __socket_SO_RCVLOWAT(b_vm *vm) {
#ifdef SO_RCVLOWAT
  return NUMBER_VAL(SO_RCVLOWAT);
#else
  return NUMBER_VAL(-1);
#endif
}

//  send timeout
b_value __socket_SO_SNDTIMEO(b_vm *vm) {
#ifdef SO_SNDTIMEO
  return NUMBER_VAL(SO_SNDTIMEO);
#else
  return NUMBER_VAL(-1);
#endif
}

//  receive timeout
b_value __socket_SO_RCVTIMEO(b_vm *vm) {
#ifdef SO_RCVTIMEO
  return NUMBER_VAL(SO_RCVTIMEO);
#else
  return NUMBER_VAL(-1);
#endif
}

//  get error status and clear
b_value __socket_SO_ERROR(b_vm *vm) {
#ifdef SO_ERROR
  return NUMBER_VAL(SO_ERROR);
#else
  return NUMBER_VAL(-1);
#endif
}

//  get socket type
b_value __socket_SO_TYPE(b_vm *vm) {
#ifdef SO_TYPE
  return NUMBER_VAL(SO_TYPE);
#else
  return NUMBER_VAL(-1);
#endif
}



//  options for socket level
b_value __socket_SOL_SOCKET(b_vm *vm) {
#ifdef SOL_SOCKET
  return NUMBER_VAL(SOL_SOCKET);
#else
  return NUMBER_VAL(-1);
#endif
}


//  unspecified
b_value __socket_AF_UNSPEC(b_vm *vm) {
#ifdef AF_UNSPEC
  return NUMBER_VAL(AF_UNSPEC);
#else
  return NUMBER_VAL(-1);
#endif
}

//  local to host (pipes)
b_value __socket_AF_UNIX(b_vm *vm) {
#ifdef AF_UNIX
  return NUMBER_VAL(AF_UNIX);
#else
  return NUMBER_VAL(-1);
#endif
}

//  same as AF_UNIX
b_value __socket_AF_LOCAL(b_vm *vm) {
#ifdef AF_LOCAL
  return NUMBER_VAL(AF_LOCAL);
#else
  return NUMBER_VAL(AF_UNIX);
#endif
}

//  internetwork: UDP, TCP, etc.
b_value __socket_AF_INET(b_vm *vm) {
#ifdef AF_INET
  return NUMBER_VAL(AF_INET);
#else
  return NUMBER_VAL(-1);
#endif
}

//  arpanet imp addresses
b_value __socket_AF_IMPLINK(b_vm *vm) {
#ifdef AF_IMPLINK
  return NUMBER_VAL(AF_IMPLINK);
#else
  return NUMBER_VAL(-1);
#endif
}

//  pup protocols: e.g. BSP
b_value __socket_AF_PUP(b_vm *vm) {
#ifdef AF_PUP
  return NUMBER_VAL(AF_PUP);
#else
  return NUMBER_VAL(-1);
#endif
}

//  mit CHAOS protocols
b_value __socket_AF_CHAOS(b_vm *vm) {
#ifdef AF_CHAOS
  return NUMBER_VAL(AF_CHAOS);
#else
  return NUMBER_VAL(-1);
#endif
}

//  XEROX NS protocols
b_value __socket_AF_NS(b_vm *vm) {
#ifdef AF_NS
  return NUMBER_VAL(AF_NS);
#else
  return NUMBER_VAL(-1);
#endif
}

//  ISO protocols
b_value __socket_AF_ISO(b_vm *vm) {
#ifdef AF_ISO
  return NUMBER_VAL(AF_ISO);
#else
  return NUMBER_VAL(-1);
#endif
}

//  OSI protocols (same as ISO)
b_value __socket_AF_OSI(b_vm *vm) {
#ifdef AF_OSI
  return NUMBER_VAL(AF_OSI);
#else
  return NUMBER_VAL(-1);
#endif
}

//  European computer manufacturers
b_value __socket_AF_ECMA(b_vm *vm) {
#ifdef AF_ECMA
  return NUMBER_VAL(AF_ECMA);
#else
  return NUMBER_VAL(-1);
#endif
}

//  datakit protocols
b_value __socket_AF_DATAKIT(b_vm *vm) {
#ifdef AF_DATAKIT
  return NUMBER_VAL(AF_DATAKIT);
#else
  return NUMBER_VAL(-1);
#endif
}

//  CITT protocols, X.25 etc
b_value __socket_AF_CCITT(b_vm *vm) {
#ifdef AF_CCITT
  return NUMBER_VAL(AF_CCITT);
#else
  return NUMBER_VAL(-1);
#endif
}

//  IBM SNA
b_value __socket_AF_SNA(b_vm *vm) {
#ifdef AF_SNA
  return NUMBER_VAL(AF_SNA);
#else
  return NUMBER_VAL(-1);
#endif
}

//  DECnet
b_value __socket_AF_DECnet(b_vm *vm) {
#ifdef AF_DECnet
  return NUMBER_VAL(AF_DECnet);
#else
  return NUMBER_VAL(-1);
#endif
}

//  DEC Direct data link interface
b_value __socket_AF_DLI(b_vm *vm) {
#ifdef AF_DLI
  return NUMBER_VAL(AF_DLI);
#else
  return NUMBER_VAL(-1);
#endif
}

//  LAT
b_value __socket_AF_LAT(b_vm *vm) {
#ifdef AF_LAT
  return NUMBER_VAL(AF_LAT);
#else
  return NUMBER_VAL(-1);
#endif
}

//  NSC Hyperchannel
b_value __socket_AF_HYLINK(b_vm *vm) {
#ifdef AF_HYLINK
  return NUMBER_VAL(AF_HYLINK);
#else
  return NUMBER_VAL(-1);
#endif
}

//  AppleTalk
b_value __socket_AF_APPLETALK(b_vm *vm) {
#ifdef AF_APPLETALK
  return NUMBER_VAL(AF_APPLETALK);
#else
  return NUMBER_VAL(-1);
#endif
}

//  ipv6
b_value __socket_AF_INET6(b_vm *vm) {
#ifdef AF_INET6
  return NUMBER_VAL(AF_INET6);
#else
  return NUMBER_VAL(-1);
#endif
}


//   Dummy protocol for TCP.
b_value __socket_IPPROTO_IP(b_vm *vm) {
#ifdef IPPROTO_IP
  return NUMBER_VAL(IPPROTO_IP);
#else
  return NUMBER_VAL(-1);
#endif
}

//   Internet Control Message Protocol.
b_value __socket_IPPROTO_ICMP(b_vm *vm) {
  return NUMBER_VAL(IPPROTO_ICMP);
}

//   Internet Group Management Protocol.
b_value __socket_IPPROTO_IGMP(b_vm *vm) {
  return NUMBER_VAL(IPPROTO_IGMP);
}

//   IPIP tunnels (older KA9Q tunnels use 94).
b_value __socket_IPPROTO_IPIP(b_vm *vm) {
#ifdef IPPROTO_IPIP
  return NUMBER_VAL(IPPROTO_IPIP);
#else
  return NUMBER_VAL(IPPROTO_IPV4);
#endif
}

//   Transmission Control Protocol.
b_value __socket_IPPROTO_TCP(b_vm *vm) {
  return NUMBER_VAL(IPPROTO_TCP);
}

//   Exterior Gateway Protocol.
b_value __socket_IPPROTO_EGP(b_vm *vm) {
#ifdef IPPROTO_EGP
  return NUMBER_VAL(IPPROTO_EGP);
#else
  return NUMBER_VAL(-1);
#endif
}

//   PUP protocol.
b_value __socket_IPPROTO_PUP(b_vm *vm) {
  return NUMBER_VAL(IPPROTO_PUP);
}

//   User Datagram Protocol.
b_value __socket_IPPROTO_UDP(b_vm *vm) {
  return NUMBER_VAL(IPPROTO_UDP);
}

//   XNS IDP protocol.
b_value __socket_IPPROTO_IDP(b_vm *vm) {
  return NUMBER_VAL(IPPROTO_IDP);
}

//   SO Transport Protocol Class 4.
b_value __socket_IPPROTO_TP(b_vm *vm) {
#ifdef IPPROTO_TP
  return NUMBER_VAL(IPPROTO_TP);
#else
  return NUMBER_VAL(-1);
#endif
}

//   Datagram Congestion Control Protocol.
b_value __socket_IPPROTO_DCCP(b_vm *vm) {
#ifdef IPPROTO_DCCP
  return NUMBER_VAL(IPPROTO_DCCP);
#else
  return NUMBER_VAL(-1);
#endif
}

//   IPv6 header.
b_value __socket_IPPROTO_IPV6(b_vm *vm) {
  return NUMBER_VAL(IPPROTO_IPV6);
}

//   Reservation Protocol.
b_value __socket_IPPROTO_RSVP(b_vm *vm) {
#ifdef IPPROTO_RSVP
  return NUMBER_VAL(IPPROTO_RSVP);
#else
  return NUMBER_VAL(-1);
#endif
}

//   General Routing Encapsulation.
b_value __socket_IPPROTO_GRE(b_vm *vm) {
#ifdef IPPROTO_GRE
  return NUMBER_VAL(IPPROTO_GRE);
#else
  return NUMBER_VAL(-1);
#endif
}

//   encapsulating security payload.
b_value __socket_IPPROTO_ESP(b_vm *vm) {
  return NUMBER_VAL(IPPROTO_ESP);
}

//   authentication header.
b_value __socket_IPPROTO_AH(b_vm *vm) {
  return NUMBER_VAL(IPPROTO_AH);
}

//   Multicast Transport Protocol.
b_value __socket_IPPROTO_MTP(b_vm *vm) {
#ifdef IPPROTO_MTP
  return NUMBER_VAL(IPPROTO_MTP);
#else
  return NUMBER_VAL(-1);
#endif
}

//   IP option pseudo header for BEET.
b_value __socket_IPPROTO_BEETPH(b_vm *vm) {
#ifdef IPPROTO_BEETPH
  return NUMBER_VAL(IPPROTO_BEETPH);
#else
  return NUMBER_VAL(-1);
#endif
}

//   Encapsulation Header.
b_value __socket_IPPROTO_ENCAP(b_vm *vm) {
#ifdef IPPROTO_ENCAP
  return NUMBER_VAL(IPPROTO_ENCAP);
#else
  return NUMBER_VAL(-1);
#endif
}

//   Protocol Independent Multicast.
b_value __socket_IPPROTO_PIM(b_vm *vm) {
  return NUMBER_VAL(IPPROTO_PIM);
}

//   Compression Header Protocol.
b_value __socket_IPPROTO_COMP(b_vm *vm) {
#ifdef IPPROTO_COMP
  return NUMBER_VAL(IPPROTO_COMP);
#else
  return NUMBER_VAL(-1);
#endif
}

//   Stream Control Transmission Protocol.
b_value __socket_IPPROTO_SCTP(b_vm *vm) {
  return NUMBER_VAL(IPPROTO_SCTP);
}

//   UDP-Lite protocol.
b_value __socket_IPPROTO_UDPLITE(b_vm *vm) {
#ifdef IPPROTO_UDPLITE
  return NUMBER_VAL(IPPROTO_UDPLITE);
#else
  return NUMBER_VAL(-1);
#endif
}

//   MPLS in IP.
b_value __socket_IPPROTO_MPLS(b_vm *vm) {
#ifdef IPPROTO_MPLS
  return NUMBER_VAL(IPPROTO_MPLS);
#else
  return NUMBER_VAL(-1);
#endif
}

//   Raw IP packets.
b_value __socket_IPPROTO_RAW(b_vm *vm) {
  return NUMBER_VAL(IPPROTO_RAW);
}

//  max IP proto
b_value __socket_IPPROTO_MAX(b_vm *vm) {
  return NUMBER_VAL(IPPROTO_MAX);
}


//  shut down the reading side
b_value __socket_SHUT_RD(b_vm *vm) {
#ifdef SHUT_RD
  return NUMBER_VAL(SHUT_RD);
#else
  return NUMBER_VAL(-1);
#endif
}

//  shut down the writing side
b_value __socket_SHUT_WR(b_vm *vm) {
#ifdef SHUT_WR
  return NUMBER_VAL(SHUT_WR);
#else
  return NUMBER_VAL(-1);
#endif
}

//  shut down both sides
b_value __socket_SHUT_RDWR(b_vm *vm) {
#ifdef SHUT_RDWR
  return NUMBER_VAL(SHUT_RDWR);
#else
  return NUMBER_VAL(-1);
#endif
}


//  Maximum queue length specifiable by listen.
b_value __socket_SOMAXCONN(b_vm *vm) {
#ifdef SOMAXCONN
  return NUMBER_VAL(SOMAXCONN);
#else
  return NUMBER_VAL(-1);
#endif
}

/** END SOCKET CONSTANTS */



CREATE_MODULE_LOADER(socket) {
  static b_func_reg module_functions[] = {
      {"_create",      false, GET_MODULE_METHOD(socket__create)},
      {"_connect",     false, GET_MODULE_METHOD(socket__connect)},
      {"_send",        false, GET_MODULE_METHOD(socket__send)},
      {"_recv",        false, GET_MODULE_METHOD(socket__recv)},
      {"_setsockopt",  false, GET_MODULE_METHOD(socket__setsockopt)},
      {"_getsockopt",  false, GET_MODULE_METHOD(socket__getsockopt)},
      {"_bind",        false, GET_MODULE_METHOD(socket__bind)},
      {"_listen",      false, GET_MODULE_METHOD(socket__listen)},
      {"_accept",      false, GET_MODULE_METHOD(socket__accept)},
      {"_error",       false, GET_MODULE_METHOD(socket__error)},
      {"_close",       false, GET_MODULE_METHOD(socket__close)},
      {"_shutdown",    false, GET_MODULE_METHOD(socket__shutdown)},
      {"_getsockinfo", false, GET_MODULE_METHOD(socket__getsockinfo)},
      {"_getaddrinfo", false, GET_MODULE_METHOD(socket__getaddrinfo)},
      {NULL,          false, NULL},
  };

  static b_field_reg socket_module_fields[] = {
      /**
       * Types
       */
      {"SOCK_STREAM", true, __socket_SOCK_STREAM},
      {"SOCK_DGRAM", true, __socket_SOCK_DGRAM},
      {"SOCK_RAW", true, __socket_SOCK_RAW},
      {"SOCK_RDM", true, __socket_SOCK_RDM},
      {"SOCK_SEQPACKET", true, __socket_SOCK_SEQPACKET},

      /**
       * Option flags per-
       */
      {"SO_DEBUG", true, __socket_SO_DEBUG},
      {"SO_ACCEPTCONN", true, __socket_SO_ACCEPTCONN},
      {"SO_REUSEADDR", true, __socket_SO_REUSEADDR},
      {"SO_KEEPALIVE", true, __socket_SO_KEEPALIVE},
      {"SO_DONTROUTE", true, __socket_SO_DONTROUTE},
      {"SO_BROADCAST", true, __socket_SO_BROADCAST},
      {"SO_USELOOPBACK", true, __socket_SO_USELOOPBACK},
      {"SO_LINGER", true, __socket_SO_LINGER},
      {"SO_OOBINLINE", true, __socket_SO_OOBINLINE},
      {"SO_REUSEPORT", true, __socket_SO_REUSEPORT},
      {"SO_TIMESTAMP", true, __socket_SO_TIMESTAMP},

      /**
       * Additional options, not kept in so_options.
       */
      {"SO_SNDBUF", true, __socket_SO_SNDBUF},
      {"SO_RCVBUF", true, __socket_SO_RCVBUF},
      {"SO_SNDLOWAT", true, __socket_SO_SNDLOWAT},
      {"SO_RCVLOWAT", true, __socket_SO_RCVLOWAT},
      {"SO_SNDTIMEO", true, __socket_SO_SNDTIMEO},
      {"SO_RCVTIMEO", true, __socket_SO_RCVTIMEO},
      {"SO_ERROR", true, __socket_SO_ERROR},
      {"SO_TYPE", true, __socket_SO_TYPE},


      {"SOL_SOCKET", true, __socket_SOL_SOCKET},

      /**
       * Address families.
       */
      {"AF_UNSPEC", true, __socket_AF_UNSPEC},
      {"AF_UNIX", true, __socket_AF_UNIX},
      {"AF_LOCAL", true, __socket_AF_LOCAL},
      {"AF_INET", true, __socket_AF_INET},
      {"AF_IMPLINK", true, __socket_AF_IMPLINK},
      {"AF_PUP", true, __socket_AF_PUP},
      {"AF_CHAOS", true, __socket_AF_CHAOS},
      {"AF_NS", true, __socket_AF_NS},
      {"AF_ISO", true, __socket_AF_ISO},
      {"AF_OSI", true, __socket_AF_OSI},
      {"AF_ECMA", true, __socket_AF_ECMA},
      {"AF_DATAKIT", true, __socket_AF_DATAKIT},
      {"AF_CCITT", true, __socket_AF_CCITT},
      {"AF_SNA", true, __socket_AF_SNA},
      {"AF_DECnet", true, __socket_AF_DECnet},
      {"AF_DLI", true, __socket_AF_DLI},
      {"AF_LAT", true, __socket_AF_LAT},
      {"AF_HYLINK", true, __socket_AF_HYLINK},
      {"AF_APPLETALK", true, __socket_AF_APPLETALK},
      {"AF_INET6", true, __socket_AF_INET6},

      /**
       * Standard well-defined IP protocols.
       */

      {"IPPROTO_IP", true, __socket_IPPROTO_IP},
      {"IPPROTO_ICMP", true, __socket_IPPROTO_ICMP},
      {"IPPROTO_IGMP", true, __socket_IPPROTO_IGMP},
      {"IPPROTO_IPIP", true, __socket_IPPROTO_IPIP},
      {"IPPROTO_TCP", true, __socket_IPPROTO_TCP},
      {"IPPROTO_EGP", true, __socket_IPPROTO_EGP},
      {"IPPROTO_PUP", true, __socket_IPPROTO_PUP},
      {"IPPROTO_UDP", true, __socket_IPPROTO_UDP},
      {"IPPROTO_IDP", true, __socket_IPPROTO_IDP},
      {"IPPROTO_TP", true, __socket_IPPROTO_TP},
      {"IPPROTO_DCCP", true, __socket_IPPROTO_DCCP},
      {"IPPROTO_IPV6", true, __socket_IPPROTO_IPV6},
      {"IPPROTO_RSVP", true, __socket_IPPROTO_RSVP},
      {"IPPROTO_GRE", true, __socket_IPPROTO_GRE},
      {"IPPROTO_ESP", true, __socket_IPPROTO_ESP},
      {"IPPROTO_AH", true, __socket_IPPROTO_AH},
      {"IPPROTO_MTP", true, __socket_IPPROTO_MTP},
      {"IPPROTO_BEETPH", true, __socket_IPPROTO_BEETPH},
      {"IPPROTO_ENCAP", true, __socket_IPPROTO_ENCAP},
      {"IPPROTO_PIM", true, __socket_IPPROTO_PIM},
      {"IPPROTO_COMP", true, __socket_IPPROTO_COMP},
      {"IPPROTO_SCTP", true, __socket_IPPROTO_SCTP},
      {"IPPROTO_UDPLITE", true, __socket_IPPROTO_UDPLITE},
      {"IPPROTO_MPLS", true, __socket_IPPROTO_MPLS},
      {"IPPROTO_RAW", true, __socket_IPPROTO_RAW},
      {"IPPROTO_MAX", true, __socket_IPPROTO_MAX},

      /**
       * howto arguments for shutdown(2), specified by Posix.1g.
       */
      {"SHUT_RD", true, __socket_SHUT_RD},
      {"SHUT_WR", true, __socket_SHUT_WR},
      {"SHUT_RDWR", true, __socket_SHUT_RDWR},

      /**
       * Maximum queue length specifiable by listen.
       */
      {"SOMAXCONN", true, __socket_SOMAXCONN},

      {NULL,       false, NULL},
  };

  static b_module_reg module = {
      .name = "_socket",
      .fields= socket_module_fields,
      .functions = module_functions,
      .classes = NULL,
      .preloader = &__socket_module_preloader,
      .unloader = &__socket_module_unload
  };
  return &module;
}

#undef BIGSIZ
#undef SMALLSIZ