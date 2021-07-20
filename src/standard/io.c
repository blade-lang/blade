#ifdef _MSC_VER
#pragma warning (disable : 4113)
#pragma warning (disable : 4047)
#pragma warning (disable : 5105)
#endif

#include "io.h"
#include "blade_unistd.h"
#include "util.h"

#ifdef IS_UNIX

#include <termios.h>

#else
#include "blade_termios.h"
#endif

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef _WIN32
#include <fcntl.h>

typedef struct COM {
  HANDLE hComm;
  int fd; // Actually it's completely useless
  char port[128];
} COM;

DCB SerialParams = {0}; // Initializing DCB structure
struct COM com;
COMMTIMEOUTS timeouts = {0}; // Initializing COMMTIMEOUTS structure

// LOCAL functions

// nbyte 0->7

int getByte(tcflag_t flag, int nbyte, int nibble) {

  int byte;
  if (nibble == 1)
    byte = (flag >> (8 * (nbyte)) & 0x0f);
  else
    byte = (flag >> (8 * (nbyte)) & 0xf0);
  return byte;
}

// INPUT FUNCTIONS

int getIXOptions(tcflag_t flag) {

#define i_IXOFF 0x01
#define i_IXON 0x02
#define i_IXOFF_IXON 0x03
#define i_PARMRK 0x04
#define i_PARMRK_IXOFF 0x05
#define i_PARMRK_IXON 0x06
#define i_PARMRK_IXON_IXOFF 0x07

  int byte = getByte(flag, 1, 1);

  return byte;
}

// LOCALOPT FUNCTIONS

int getEchoOptions(tcflag_t flag) {

#define l_NOECHO 0x00
#define l_ECHO 0x01
#define l_ECHO_ECHOE 0x03
#define l_ECHO_ECHOK 0x05
#define l_ECHO_ECHONL 0x09
#define l_ECHO_ECHOE_ECHOK 0x07
#define l_ECHO_ECHOE_ECHONL 0x0b
#define l_ECHO_ECHOE_ECHOK_ECHONL 0x0f
#define l_ECHO_ECHOK_ECHONL 0x0d
#define l_ECHOE 0x02
#define l_ECHOE_ECHOK 0x06
#define l_ECHOE_ECHONL 0x0a
#define l_ECHOE_ECHOK_ECHONL 0x0e
#define l_ECHOK 0x04
#define l_ECHOK_ECHONL 0x0c
#define l_ECHONL 0x08

  int byte = getByte(flag, 1, 1);
  return byte;
}

int getLocalOptions(tcflag_t flag) {

#define l_ICANON 0x10
#define l_ICANON_ISIG 0x50
#define l_ICANON_IEXTEN 0x30
#define l_ICANON_NOFLSH 0x90
#define l_ICANON_ISIG_IEXTEN 0x70
#define l_ICANON_ISIG_NOFLSH 0xd0
#define l_ICANON_IEXTEN_NOFLSH 0xb0
#define l_ICANON_ISIG_IEXTEN_NOFLSH 0xf0
#define l_ISIG 0x40
#define l_ISIG_IEXTEN 0x60
#define l_ISIG_NOFLSH 0xc0
#define l_ISIG_IEXTEN_NOFLSH 0xe0
#define l_IEXTEN 0x20
#define l_IEXTEN_NOFLSH 0xa0
#define l_NOFLSH 0x80

  int byte = getByte(flag, 1, 0);
  return byte;
}

int getToStop(tcflag_t flag) {

#define l_TOSTOP 0x01

  int byte = getByte(flag, 1, 1);
  return byte;
}

// CONTROLOPT FUNCTIONS

int getCharSet(tcflag_t flag) {

  // FLAG IS MADE UP OF 8 BYTES, A FLAG IS MADE UP OF A NIBBLE -> 4 BITS, WE
  // NEED TO EXTRACT THE SECOND NIBBLE (1st) FROM THE FIFTH BYTE (6th).
  int byte = getByte(flag, 1, 1);

  switch (byte) {

  case 0X0:
    return CS5;
    break;

  case 0X4:
    return CS6;
    break;

  case 0X8:
    return CS7;
    break;

  case 0Xc:
    return CS8;
    break;

  default:
    return CS8;
    break;
  }
}

int getControlOptions(tcflag_t flag) {

#define c_ALL_ENABLED 0xd0
#define c_PAREVEN_CSTOPB 0x50
#define c_PAREVEN_NOCSTOPB 0x40
#define c_PARODD_NOCSTOPB 0xc0
#define c_NOPARENB_CSTOPB 0x10
#define c_ALL_DISABLED 0x00

  int byte = getByte(flag, 1, 0);
  return byte;
}

// LIBFUNCTIONS

int tcgetattr(int fd, struct termios *termios_p) {

  if (fd != com.fd)
    return -1;
  return GetCommState(com.hComm, &SerialParams);
}

int tcsetattr(int fd, int optional_actions, const struct termios *termios_p) {

  if (fd != com.fd)
    return -1;
  int ret = 0;

  // Store flags into local variables
  tcflag_t iflag = termios_p->c_iflag;
  // tcflag_t lflag = termios_p->c_lflag;
  tcflag_t cflag = termios_p->c_cflag;
  // tcflag_t oflag = termios_p->c_oflag;

  // iflag

  int IX = getIXOptions(iflag);

  if ((IX == i_IXOFF_IXON) || (IX == i_PARMRK_IXON_IXOFF)) {

    SerialParams.fOutX = TRUE;
    SerialParams.fInX = TRUE;
    SerialParams.fTXContinueOnXoff = TRUE;
  }

  // lflag
  // int EchoOpt = getEchoOptions(lflag);
  // int l_opt = getLocalOptions(lflag);
  // int tostop = getToStop(lflag);

  // Missing parameters...

  // cflags

  int CharSet = getCharSet(cflag);
  int c_opt = getControlOptions(cflag);

  switch (CharSet) {

  case CS5:
    SerialParams.ByteSize = 5;
    break;

  case CS6:
    SerialParams.ByteSize = 6;
    break;

  case CS7:
    SerialParams.ByteSize = 7;
    break;

  case CS8:
    SerialParams.ByteSize = 8;
    break;
  }

  switch (c_opt) {

  case c_ALL_ENABLED:
    SerialParams.Parity = ODDPARITY;
    SerialParams.StopBits = TWOSTOPBITS;
    break;

  case c_ALL_DISABLED:
    SerialParams.Parity = NOPARITY;
    SerialParams.StopBits = ONESTOPBIT;
    break;

  case c_PAREVEN_CSTOPB:
    SerialParams.Parity = EVENPARITY;
    SerialParams.StopBits = TWOSTOPBITS;
    break;

  case c_PAREVEN_NOCSTOPB:
    SerialParams.Parity = EVENPARITY;
    SerialParams.StopBits = ONESTOPBIT;
    break;

  case c_PARODD_NOCSTOPB:
    SerialParams.Parity = ODDPARITY;
    SerialParams.StopBits = ONESTOPBIT;
    break;

  case c_NOPARENB_CSTOPB:
    SerialParams.Parity = NOPARITY;
    SerialParams.StopBits = TWOSTOPBITS;
    break;
  }

  // aflags

  /*
  int OP;
  if(oflag == OPOST)
  else ...
  */
  // Missing parameters...

  // special characters

  if (termios_p->c_cc[VEOF] != 0)
    SerialParams.EofChar = (char)termios_p->c_cc[VEOF];
  if (termios_p->c_cc[VINTR] != 0)
    SerialParams.EvtChar = (char)termios_p->c_cc[VINTR];

  if (termios_p->c_cc[VMIN] == 1) { // Blocking

    timeouts.ReadIntervalTimeout = 0;         // in milliseconds
    timeouts.ReadTotalTimeoutConstant = 0;    // in milliseconds
    timeouts.ReadTotalTimeoutMultiplier = 0;  // in milliseconds
    timeouts.WriteTotalTimeoutConstant = 0;   // in milliseconds
    timeouts.WriteTotalTimeoutMultiplier = 0; // in milliseconds

  } else { // Non blocking

    timeouts.ReadIntervalTimeout =
        termios_p->c_cc[VTIME] * 100; // in milliseconds
    timeouts.ReadTotalTimeoutConstant =
        termios_p->c_cc[VTIME] * 100; // in milliseconds
    timeouts.ReadTotalTimeoutMultiplier =
        termios_p->c_cc[VTIME] * 100; // in milliseconds
    timeouts.WriteTotalTimeoutConstant =
        termios_p->c_cc[VTIME] * 100; // in milliseconds
    timeouts.WriteTotalTimeoutMultiplier =
        termios_p->c_cc[VTIME] * 100; // in milliseconds
  }

  SetCommTimeouts(com.hComm, &timeouts);

  // EOF

  ret = SetCommState(com.hComm, &SerialParams);
  if (ret != 0)
    return 0;
  else
    return -1;
}

int tcsendbreak(int fd, int duration) {

  if (fd != com.fd)
    return -1;

  int ret = 0;
  ret = TransmitCommChar(com.hComm, '\x00');
  if (ret != 0)
    return 0;
  else
    return -1;
}

int tcdrain(int fd) {

  if (fd != com.fd)
    return -1;
  return FlushFileBuffers(com.hComm);
}

int tcflush(int fd, int queue_selector) {

  if (fd != com.fd)
    return -1;
  int rc = 0;

  switch (queue_selector) {

  case TCIFLUSH:
    rc = PurgeComm(com.hComm, PURGE_RXCLEAR);
    break;

  case TCOFLUSH:
    rc = PurgeComm(com.hComm, PURGE_TXCLEAR);
    break;

  case TCIOFLUSH:
    rc = PurgeComm(com.hComm, PURGE_RXCLEAR);
    rc *= PurgeComm(com.hComm, PURGE_TXCLEAR);
    break;

  default:
    rc = 0;
    break;
  }

  if (rc != 0)
    return 0;
  else
    return -1;
}

int tcflow(int fd, int action) {

  if (fd != com.fd)
    return -1;
  int rc = 0;

  switch (action) {

  case TCOOFF:
    rc = PurgeComm(com.hComm, PURGE_TXABORT);
    break;

  case TCOON:
    rc = ClearCommBreak(com.hComm);
    break;

  case TCIOFF:
    rc = PurgeComm(com.hComm, PURGE_RXABORT);
    break;

  case TCION:
    rc = ClearCommBreak(com.hComm);
    break;

  default:
    rc = 0;
    break;
  }

  if (rc != 0)
    return 0;
  else
    return -1;
}

void cfmakeraw(struct termios *termios_p) {

  SerialParams.ByteSize = 8;
  SerialParams.StopBits = ONESTOPBIT;
  SerialParams.Parity = NOPARITY;
}

speed_t cfgetispeed(const struct termios *termios_p) {

  return SerialParams.BaudRate;
}

speed_t cfgetospeed(const struct termios *termios_p) {

  return SerialParams.BaudRate;
}

int cfsetispeed(struct termios *termios_p, speed_t speed) {

  SerialParams.BaudRate = speed;
  return 0;
}

int cfsetospeed(struct termios *termios_p, speed_t speed) {

  SerialParams.BaudRate = speed;
  return 0;
}

int cfsetspeed(struct termios *termios_p, speed_t speed) {

  SerialParams.BaudRate = speed;
  return 0;
}

ssize_t read_serial(int fd, void *buffer, size_t count) {

  if (fd != com.fd)
    return -1;
  long unsigned int rc = 0;
  int ret;

  ret = ReadFile(com.hComm, buffer, count, &rc, NULL);

  if (ret == 0)
    return -1;
  else
    return (ssize_t)rc;
}

ssize_t write_serial(int fd, const void *buffer, size_t count) {

  if (fd != com.fd)
    return -1;
  long unsigned int rc = 0;
  int ret;

  ret = WriteFile(com.hComm, buffer, count, &rc, NULL);

  if (ret == 0)
    return -1;
  else
    return (ssize_t)rc;
}

int open_serial(const char *portname, int opt) {

  if (strlen(portname) < 4)
    return -1;

  // Set to zero
  memset(com.port, 0x00, 128);

  // COMxx
  size_t portSize = 0;
  if (strlen(portname) > 4) {
    portSize = sizeof(char) * strlen("\\\\.\\COM10") + 1;
#ifdef _MSC_VER
    strncat_s(com.port, portSize, "\\\\.\\", strlen("\\\\.\\"));
#else
    strncat(com.port, "\\\\.\\", strlen("\\\\.\\") - 1);
#endif
  }
  // COMx
  else {
    portSize = sizeof(char) * 5;
  }

#ifdef _MSC_VER
  strncat_s(com.port, portSize, portname, 4);
#else
  strncat(com.port, portname, 4);
#endif
  com.port[portSize] = 0x00;

  switch (opt) {

  case O_RDWR:
    com.hComm = CreateFile(com.port, GENERIC_READ | GENERIC_WRITE, 0, NULL,
                           OPEN_EXISTING, 0, NULL);
    break;

  case O_RDONLY:
    com.hComm =
        CreateFile(com.port, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
    break;

  case O_WRONLY:
    com.hComm =
        CreateFile(com.port, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
    break;
  }

  if (com.hComm == INVALID_HANDLE_VALUE) {
    return -1;
  }
  com.fd = atoi(portname + 3); // COMx and COMxx
  SerialParams.DCBlength = sizeof(SerialParams);
  return com.fd;
}

int close_serial(int fd) {

  int ret = CloseHandle(com.hComm);
  if (ret != 0)
    return 0;
  else
    return -1;
}

int select_serial(int nfds, fd_set *readfds, fd_set *writefds,
                  fd_set *exceptfds, struct timeval *timeout) {

  SetCommMask(com.hComm, EV_RXCHAR);
  DWORD dwEventMask = NULL;
  if (WaitCommEvent(com.hComm, &dwEventMask, NULL) == 0) {
    return -1; // Return -1 if failed
  }
  if (dwEventMask == EV_RXCHAR) {
    return com.fd;
  } else {
    if (readfds) {
      // Clear file descriptor if event is not RXCHAR
      FD_CLR(com.fd, readfds);
    }
  }
  // NOTE: write event not detectable!
  // NOTE: no timeout
  return 0; // No data
}

// Returns hComm from the COM structure
HANDLE getHandle() { return com.hComm; }

#endif

static struct termios orig_termios;
void disable_raw_mode(void) {
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

/**
 * tty._tcgetattr()
 *
 * returns the configuration of the current tty input
 */
DECLARE_MODULE_METHOD(io_tty__tcgetattr) {
  ENFORCE_ARG_COUNT(_tcgetattr, 1);
  ENFORCE_ARG_TYPE(_tcsetattr, 0, IS_FILE);

  b_obj_file *file = AS_FILE(args[0]);

  if (!is_std_file(file)) {
    RETURN_ERROR("can only use tty on std objects");
  }

  struct termios raw_attr;
  if (tcgetattr(fileno(file->file), &raw_attr) != 0) {
    RETURN_ERROR(strerror(errno));
  }

  // we have our attributes already
  b_obj_dict *dict = new_dict(vm);
  dict_add_entry(vm, dict, NUMBER_VAL(0), NUMBER_VAL(raw_attr.c_iflag));
  dict_add_entry(vm, dict, NUMBER_VAL(1), NUMBER_VAL(raw_attr.c_oflag));
  dict_add_entry(vm, dict, NUMBER_VAL(2), NUMBER_VAL(raw_attr.c_cflag));
  dict_add_entry(vm, dict, NUMBER_VAL(3), NUMBER_VAL(raw_attr.c_lflag));
  dict_add_entry(vm, dict, NUMBER_VAL(4), NUMBER_VAL(raw_attr.c_ispeed));
  dict_add_entry(vm, dict, NUMBER_VAL(5), NUMBER_VAL(raw_attr.c_ospeed));

  RETURN_OBJ(dict);
}

/**
 * tty._tcsetattr(attrs: dict)
 *
 * sets the attributes of a tty
 * @return true if succeed or false otherwise
 * TODO: support the c_cc flag
 */
DECLARE_MODULE_METHOD(io_tty__tcsetattr) {
  ENFORCE_ARG_COUNT(_tcsetattr, 3);
  ENFORCE_ARG_TYPE(_tcsetattr, 0, IS_FILE);
  ENFORCE_ARG_TYPE(_tcsetattr, 1, IS_NUMBER);
  ENFORCE_ARG_TYPE(_tcsetattr, 2, IS_DICT);

  b_obj_file *file = AS_FILE(args[0]);
  int type = AS_NUMBER(args[1]);
  b_obj_dict *dict = AS_DICT(args[2]);

  if (!is_std_file(file)) {
    RETURN_ERROR("can only use tty on std objects");
  }

  if (type < 0) {
    RETURN_ERROR("tty options should be one of TTY's TCSA");
  }

  // make sure we have good values so that we don't freeze the tty
  for (int i = 0; i < dict->names.count; i++) {
    if (!IS_NUMBER(dict->names.values[i]) ||
        AS_NUMBER(dict->names.values[i]) < 0 || // c_iflag
        AS_NUMBER(dict->names.values[i]) > 5) { // ospeed
      RETURN_ERROR("attributes must be one of io TTY flags");
    }
    b_value dummy_value;
    if (dict_get_entry(dict, dict->names.values[i], &dummy_value)) {
      if (!IS_NUMBER(dummy_value)) {
        RETURN_ERROR("TTY attribute cannot be %s", value_type(dummy_value));
      }
    }
  }

  b_value iflag = NIL_VAL, oflag = NIL_VAL, cflag = NIL_VAL, lflag = NIL_VAL,
      ispeed = NIL_VAL, ospeed = NIL_VAL;


  tcgetattr(STDIN_FILENO, &orig_termios);
  atexit(disable_raw_mode);

  struct termios raw = orig_termios;

  if (dict_get_entry(dict, NUMBER_VAL(0), &iflag)) {
    raw.c_iflag = (long) AS_NUMBER(iflag);
  }
  if (dict_get_entry(dict, NUMBER_VAL(1), &iflag)) {
    raw.c_oflag = (long) AS_NUMBER(oflag);
  }
  if (dict_get_entry(dict, NUMBER_VAL(2), &iflag)) {
    raw.c_cflag = (long) AS_NUMBER(cflag);
  }
  if (dict_get_entry(dict, NUMBER_VAL(3), &iflag)) {
    raw.c_lflag = (long) AS_NUMBER(lflag);
  }
  if (dict_get_entry(dict, NUMBER_VAL(4), &iflag)) {
    raw.c_ispeed = (long) AS_NUMBER(ispeed);
  }
  if (dict_get_entry(dict, NUMBER_VAL(5), &iflag)) {
    raw.c_ospeed = (long) AS_NUMBER(ospeed);
  }

  int result = tcsetattr(fileno(file->file), type, &raw);
  RETURN_BOOL( result != -1);
}

/**
 * TTY.flush()
 * flushes the standard output and standard error interface
 * @return nil
 */
DECLARE_MODULE_METHOD(io_tty__flush) {
  ENFORCE_ARG_COUNT(TTY.flush, 0);
  fflush(stdout);
  fflush(stderr);
  RETURN;
}

/**
 * flush()
 * flushes the given file handle
 * @return nil
 */
DECLARE_MODULE_METHOD(io_flush) {
  ENFORCE_ARG_COUNT(flush, 1);
  ENFORCE_ARG_TYPE(flush, 0, IS_FILE);
  b_obj_file *file = AS_FILE(args[0]);

  if(file->is_open) {
    fflush(file->file);
  }
  RETURN;
}

/**
 * TTY.flush()
 * flushes the standard output and standard error interface
 * @return nil
 */
DECLARE_MODULE_METHOD(io_tty__exit_raw) {
  ENFORCE_ARG_COUNT(TTY.exit_raw,  0);
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
  RETURN;
}

/**
 * getc()
 *
 * reads character(s) from standard input
 *
 * when length is given, gets `length` number of characters
 * else, gets a single character
 * @returns char
 */
DECLARE_MODULE_METHOD(io_getc) {
  ENFORCE_ARG_RANGE(getc, 0, 1);

  int length = 1;
  if (arg_count == 1) {
    ENFORCE_ARG_TYPE(getc, 0, IS_NUMBER);
    length = AS_NUMBER(args[0]);
  }

  char *result = ALLOCATE(char, (size_t)length + 2);
  read_line(result, length + 1);
  RETURN_L_STRING(result, length);
}

/**
 * putc(c: char)
 * writes character c to the screen
 * @return nil
 */
DECLARE_MODULE_METHOD(io_putc) {
  ENFORCE_ARG_COUNT(putc, 1);
  ENFORCE_ARG_TYPE(putc, 0, IS_STRING);

  b_obj_string *string = AS_STRING(args[0]);

  int count = string->length;
#ifdef _WIN32
  if (count > 32767 && isatty(STDIN_FILENO)) {
    /* Issue #11395: the Windows console returns an error (12: not
       enough space error) on writing into stdout if stdout mode is
       binary and the length is greater than 66,000 bytes (or less,
       depending on heap usage). */
    count = 32767;
  }
#endif

  if(write(STDOUT_FILENO, string->chars, count) != -1) {
    fflush(stdout);
  }
  RETURN;
}

/**
 * stdin()
 *
 * returns the standard input
 */
b_value io_module_stdin(b_vm *vm) {
  b_obj_file *file =
      new_file(vm, copy_string(vm, "<stdin>", 7), copy_string(vm, "", 0));
  file->file = stdin;
  file->is_open = true;
  file->mode = copy_string(vm, "", 0);
  return OBJ_VAL(file);
}

/**
 * stdout()
 *
 * returns the standard output interface
 */
b_value io_module_stdout(b_vm *vm) {
  b_obj_file *file =
      new_file(vm, copy_string(vm, "<stdout>", 8), copy_string(vm, "", 0));
  file->file = stdout;
  file->is_open = true;
  file->mode = copy_string(vm, "", 0);
  return OBJ_VAL(file);
}

/**
 * stderr()
 *
 * returns the standard error interface
 */
b_value io_module_stderr(b_vm *vm) {
  b_obj_file *file =
      new_file(vm, copy_string(vm, "<stdout>", 8), copy_string(vm, "", 0));
  file->file = stderr;
  file->is_open = true;
  file->mode = copy_string(vm, "", 0);
  return OBJ_VAL(file);
}

CREATE_MODULE_LOADER(io) {
  static b_field_reg io_module_fields[] = {
      {"stdin",       false, io_module_stdin},
      {"stdout",       false, io_module_stdout},
      {"stderr",       false, io_module_stderr},
      {NULL,       false, NULL},
  };

  static b_func_reg io_functions[] = {
      {"getc",   false, GET_MODULE_METHOD(io_getc)},
      {"putc",   false, GET_MODULE_METHOD(io_putc)},
      {"flush",   false, GET_MODULE_METHOD(io_flush)},
      {NULL,     false, NULL},
  };

  static b_func_reg tty_class_functions[] = {
      {"tcgetattr", false, GET_MODULE_METHOD(io_tty__tcgetattr)},
      {"tcsetattr", false, GET_MODULE_METHOD(io_tty__tcsetattr)},
      {"flush",     false, GET_MODULE_METHOD(io_tty__flush)},
      {"exit_raw",     false, GET_MODULE_METHOD(io_tty__exit_raw)},
      {NULL,         false, NULL},
  };

  static b_class_reg classes[] = {
      {"TTY", NULL, tty_class_functions},
      {NULL,  NULL, NULL},
  };

  static b_module_reg module = {"_io", io_module_fields, io_functions, classes};

  return module;
}