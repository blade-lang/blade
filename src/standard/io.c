#ifdef _MSC_VER
#pragma warning (disable : 4113)
#pragma warning (disable : 4047)
#pragma warning (disable : 5105)
#endif

#include "module.h"
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#else
#include "blade_unistd.h"
#endif /* HAVE_UNISTD_H */
#include "util.h"

#include <errno.h>
#include <stdio.h>

#ifdef HAVE_TERMIOS_H
#include <termios.h>
#include <stdlib.h>

static struct termios orig_termios;
static bool set_attr_was_called = false;

void disable_raw_mode(void) {
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}
#endif /* HAVE_TERMIOS_H */

/**
 * tty._tcgetattr()
 *
 * returns the configuration of the current tty input
 */
DECLARE_MODULE_METHOD(io_tty__tcgetattr) {
  ENFORCE_ARG_COUNT(_tcgetattr, 1);
  ENFORCE_ARG_TYPE(_tcsetattr, 0, IS_FILE);

#ifdef HAVE_TERMIOS_H
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
#else
  RETURN_ERROR("tcgetattr() is not supported on this platform");
#endif /* HAVE_TERMIOS_H */
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

#ifdef HAVE_TERMIOS_H
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

  set_attr_was_called = true;

  int result = tcsetattr(fileno(file->file), type, &raw);
  RETURN_BOOL(result != -1);
#else
  RETURN_ERROR("tcsetattr() is not supported on this platform");
#endif /* HAVE_TERMIOS_H */
}

/**
 * TTY.exit_raw()
 * exits raw mode
 * @return nil
 */
DECLARE_MODULE_METHOD(io_tty__exit_raw) {
#ifdef HAVE_TERMIOS_H
  ENFORCE_ARG_COUNT(TTY.exit_raw, 0);
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
  RETURN;
#else
  RETURN_ERROR("exit_raw() is not supported on this platform");
#endif /* HAVE_TERMIOS_H */
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

  if (file->is_open) {
    fflush(file->file);
  }
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

  char *result = ALLOCATE(char, (size_t) length + 2);
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

  if (write(STDOUT_FILENO, string->chars, count) != -1) {
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

void __io_module_unload(b_vm *vm) {
#ifdef HAVE_TERMIOS_H
  if (set_attr_was_called) {
    disable_raw_mode();
  }
#endif /* ifdef HAVE_TERMIOS_H */
}

CREATE_MODULE_LOADER(io) {
  static b_field_reg io_module_fields[] = {
      {"stdin",  false, io_module_stdin},
      {"stdout", false, io_module_stdout},
      {"stderr", false, io_module_stderr},
      {NULL,     false, NULL},
  };

  static b_func_reg io_functions[] = {
      {"getc",  false, GET_MODULE_METHOD(io_getc)},
      {"putc",  false, GET_MODULE_METHOD(io_putc)},
      {"flush", false, GET_MODULE_METHOD(io_flush)},
      {NULL,    false, NULL},
  };

  static b_func_reg tty_class_functions[] = {
      {"tcgetattr", false, GET_MODULE_METHOD(io_tty__tcgetattr)},
      {"tcsetattr", false, GET_MODULE_METHOD(io_tty__tcsetattr)},
      {"flush",     false, GET_MODULE_METHOD(io_tty__flush)},
      {"exit_raw",  false, GET_MODULE_METHOD(io_tty__exit_raw)},
      {NULL,        false, NULL},
  };

  static b_class_reg classes[] = {
      {"TTY", NULL, tty_class_functions},
      {NULL,  NULL, NULL},
  };

  static b_module_reg module = {
      .name = "_io",
      .fields = io_module_fields,
      .functions = io_functions,
      .classes = classes,
      .preloader = NULL,
      .unloader = &__io_module_unload
  };

  return &module;
}