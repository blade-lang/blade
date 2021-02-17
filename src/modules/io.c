#include "io.h"
#include "compat/unistd.h"
#include "util.h"

#ifdef IS_UNIX
#include <termios.h>
#endif

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
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
  int status;
  if ((status = tcgetattr(fileno(file->file), &raw_attr)) != 0) {
    switch (status) {
    case ENOTTY:
      RETURN_ERROR("stdin is not a TTY");
      break;
    case EBADF:
      RETURN_ERROR("stdin is a bad file descriptor");
      break;
    }
    RETURN;
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

  struct termios raw;
  tcgetattr(fileno(file->file), &raw);

  if (dict_get_entry(dict, NUMBER_VAL(0), &iflag)) {
    raw.c_iflag = (long)AS_NUMBER(iflag);
  }
  if (dict_get_entry(dict, NUMBER_VAL(1), &iflag)) {
    raw.c_oflag = (long)AS_NUMBER(oflag);
  }
  if (dict_get_entry(dict, NUMBER_VAL(2), &iflag)) {
    raw.c_cflag = (long)AS_NUMBER(cflag);
  }
  if (dict_get_entry(dict, NUMBER_VAL(3), &iflag)) {
    raw.c_lflag = (long)AS_NUMBER(lflag);
  }
  if (dict_get_entry(dict, NUMBER_VAL(4), &iflag)) {
    raw.c_ispeed = (long)AS_NUMBER(ispeed);
  }
  if (dict_get_entry(dict, NUMBER_VAL(5), &iflag)) {
    raw.c_ospeed = (long)AS_NUMBER(ospeed);
  }

  RETURN_BOOL(tcsetattr(fileno(file->file), type, &raw) != -1);
}

/**
 * TTY.flush()
 *
 * flushes the standard output and standard error interface
 */
DECLARE_MODULE_METHOD(io_tty__flush) {
  ENFORCE_ARG_COUNT(TTY.flush, 0);
  fflush(stdout);
  fflush(stderr);
  RETURN;
}

/**
 * getc()
 *
 * reads a single character from standard input
 * @returns char
 */
DECLARE_NATIVE(io_getc) {
  ENFORCE_ARG_RANGE(getc, 0, 1);

  int length = 1;
  if (arg_count == 1) {
    ENFORCE_ARG_TYPE(getc, 0, IS_NUMBER);
    length = AS_NUMBER(args[0]);
  }

  int nread;
  char c[length];
  while ((nread = read(STDIN_FILENO, c, length)) != 1) {
    if (nread == -1 && errno != EAGAIN) {
      RETURN_ERROR("error reading character from stdin");
    }
  }

  if (length == 1) {
    char *ch = utf8_encode(c[0]);
    RETURN_STRING(ch);
  } else {
    char result[length + 1];
    length = read_line(result, length + 1);
    RETURN_LSTRING(result, length);
  }
}

/**
 * putc(c: char)
 *
 * writes character c to the screen
 */
DECLARE_NATIVE(io_putc) {
  ENFORCE_ARG_COUNT(putc, 1);
  ENFORCE_ARG_TYPE(putc, 0, IS_STRING);

  b_obj_string *string = AS_STRING(args[0]);

  int count = string->length;
#ifdef IS_WINDOWS
  if (count > 32767 && isatty(STDIN_FILENO)) {
    /* Issue #11395: the Windows console returns an error (12: not
       enough space error) on writing into stdout if stdout mode is
       binary and the length is greater than 66,000 bytes (or less,
       depending on heap usage). */
    count = 32767;
  }
#endif

  write(STDOUT_FILENO, string->chars, count);
  fflush(stdout);
  RETURN;
}

/**
 * stdin()
 *
 * returns the standard input
 */
DECLARE_NATIVE(io_stdin) {
  ENFORCE_ARG_COUNT(stdin, 0);
  b_obj_file *file =
      new_file(vm, copy_string(vm, "<stdin>", 7), copy_string(vm, "", 0));
  file->file = stdin;
  file->is_open = true;
  RETURN_OBJ(file);
}

/**
 * stdout()
 *
 * returns the standard output interface
 */
DECLARE_NATIVE(io_stdout) {
  ENFORCE_ARG_COUNT(stdout, 0);
  b_obj_file *file =
      new_file(vm, copy_string(vm, "<stdout>", 8), copy_string(vm, "", 0));
  file->file = stdout;
  file->is_open = true;
  RETURN_OBJ(file);
}

/**
 * stderr()
 *
 * returns the standard error interface
 */
DECLARE_NATIVE(io_stderr) {
  ENFORCE_ARG_COUNT(stderr, 0);
  b_obj_file *file =
      new_file(vm, copy_string(vm, "<stderr>", 8), copy_string(vm, "", 0));
  file->file = stderr;
  file->is_open = true;
  RETURN_OBJ(file);
}

static b_func_reg io_functions[] = {
    {"getc", false, GET_NATIVE(io_getc)},
    {"putc", false, GET_NATIVE(io_putc)},
    {"stdin", false, GET_NATIVE(io_stdin)},
    {"stdout", false, GET_NATIVE(io_stdout)},
    {"stderr", false, GET_NATIVE(io_stderr)},
    {NULL, false, NULL},
};

static b_func_reg tty_class_functions[] = {
    {"_tcgetattr", false, GET_MODULE_METHOD(io_tty__tcgetattr)},
    {"_tcsetattr", false, GET_MODULE_METHOD(io_tty__tcsetattr)},
    {"_flush", false, GET_MODULE_METHOD(io_tty__flush)},
    {NULL, false, NULL},
};

static b_class_reg klasses[] = {
    {"TTY", tty_class_functions},
    {NULL, NULL},
};

static b_module_reg module = {io_functions, klasses};

CREATE_MODULE_LOADER(io) { return module; }