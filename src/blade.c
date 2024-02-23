#include "util.h"
#include "vm.h"

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#else
#include "bunistd.h"
#endif /* ifdef HAVE_UNISTD_H */
#include "module.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#if !defined(_WIN32) && !defined(__CYGWIN__)
#include "linenoise/utf8.h"
#include "linenoise/linenoise.h"
#endif

#include <setjmp.h>
#include <signal.h>
#include <fcntl.h>

#ifdef _WIN32
#include <windows.h>
#include "pathinfo.h"
#endif

static void repl(b_vm *vm) {
  vm->is_repl = true;
  bool continue_repl = true;

  printf("Blade %s (running on BladeVM %s), REPL/Interactive mode = ON\n",
         BLADE_VERSION_STRING, BVM_VERSION);
  printf("%s, (Build time = %s, %s)\n", COMPILER, __DATE__, __TIME__);
  printf("Type \".exit\" to quit or \".credits\" for more information\n");

  char *source = (char *) malloc(sizeof(char));
  memset(source, 0, sizeof(char));

  int brace_count = 0, paren_count = 0, bracket_count = 0, single_quote_count = 0, double_quote_count = 0;

  b_obj_module *module = new_module(vm, strdup(""), strdup("<repl>"));
  add_module(vm, module);
  register_module__FILE__(vm, module);

#if !defined(_WIN32) && !defined(__CYGWIN__)

  linenoiseSetEncodingFunctions(
      linenoiseUtf8PrevCharLen,
      linenoiseUtf8NextCharLen,
      linenoiseUtf8ReadCode
  );
  linenoiseSetMultiLine(0);

  // allow user to navigate through past input in terminal...
  linenoiseHistoryAdd(".exit");
#endif // !_WIN32

  for (;;) {

    if (!continue_repl) {
      brace_count = 0;
      paren_count = 0;
      bracket_count = 0;
      single_quote_count = 0;
      double_quote_count = 0;

      // reset source...
      memset(source, 0, strlen(source));
      continue_repl = true;
    }

    const char *cursor = "%> ";
    if (brace_count > 0 || bracket_count > 0 || paren_count > 0) {
      cursor = ".. ";
    } else if (single_quote_count == 1 || double_quote_count == 1) {
      cursor = "";
    }

#if defined(_WIN32) || defined(__CYGWIN__)
    char buffer[1024];
    printf(cursor);
    char *line = fgets(buffer, 1024, stdin);

    int line_length = 0;
    if(line != NULL) {
      line_length = strcspn(line, "\r\n");
      line[line_length] = 0;
    }

    // terminate early if we receive a terminating command such as exit() or Ctrl+D
    if(line == NULL || strcmp(line, ".exit") == 0) {
      free(source);
      return;
    }
#else
    char *line = linenoise(cursor);

    // terminate early if we receive a terminating command such as exit() or Ctrl+D
    if (line == NULL || strcmp(line, ".exit") == 0) {
      free(source);
      return;
    }

    int line_length = (int) strlen(line);
#endif // _WIN32

    if(strcmp(line, ".credits") == 0) {
      printf("\n" BLADE_COPYRIGHT "\n\n");
      memset(source, 0, sizeof(char));
      continue;
    }

#if !defined(_WIN32) && !defined(__CYGWIN__)
    // allow user to navigate through past input in terminal...
    linenoiseHistoryAdd(line);
#endif // !_WIN32

    if(line_length > 0 && line[0] == '#') {
      continue;
    }

    // find count of { and }, ( and ), [ and ], " and '
    for (int i = 0; i < line_length; i++) {
      // scope openers...
      if (line[i] == '{')
        brace_count++;
      if (line[i] == '(')
        paren_count++;
      if (line[i] == '[')
        bracket_count++;

      // quotes
      if (line[i] == '\'' && double_quote_count == 0) {
        if (single_quote_count == 0) single_quote_count++;
        else single_quote_count--;
      }
      if (line[i] == '"' && single_quote_count == 0) {
        if (double_quote_count == 0)double_quote_count++;
        else double_quote_count--;
      }

      if (line[i] == '\\' && (single_quote_count > 0 || double_quote_count > 0)) i++;

      // scope closers...
      if (line[i] == '}' && brace_count > 0)
        brace_count--;
      if (line[i] == ')' && paren_count > 0)
        paren_count--;
      if (line[i] == ']' && bracket_count > 0)
        bracket_count--;
    }

    source = append_strings(source, line);
    if (line_length > 0) {
      source = append_strings(source, "\n");
    }

#ifndef _WIN32
    linenoiseFree(line);
#endif // !_WIN32

    if (bracket_count == 0 && paren_count == 0 && brace_count == 0 && single_quote_count == 0 &&
        double_quote_count == 0) {

      interpret(vm, module, source);

      fflush(stdout); // flush all outputs

      // reset source...
      continue_repl = false;
    }
  }
}

static void run_file(b_vm *vm, char *file) {
  char *source = read_file(file);
  if (source == NULL) {
    // check if it's a Blade library directory by attempting to read the index file.
    char *old_file = file;
    file = append_strings((char *)strdup(file), "/" LIBRARY_DIRECTORY_INDEX BLADE_EXTENSION);
    source = read_file(file);

    if(source == NULL) {
      fprintf(stderr, "(Blade):\n  Launch aborted for %s\n  Reason: %s\n", old_file, strerror(errno));
      exit(EXIT_FAILURE);
    }
  }

  // set root file...
  vm->root_file = file;

  b_obj_module *module = new_module(vm, strdup(""), realpath(file, NULL));
  add_module(vm, module);
  register_module__FILE__(vm, module);

  b_ptr_result result = interpret(vm, module, source);
  free(source);

  fflush(stdout);

  if (result == PTR_COMPILE_ERR)
    exit(EXIT_COMPILE);
  if (result == PTR_RUNTIME_ERR)
    exit(EXIT_RUNTIME);
}

static void run_code(b_vm *vm, char *source) {
  // set root file...
  vm->root_file = NULL;

  b_obj_module *module = new_module(vm, strdup(""), strdup("<script>"));
  add_module(vm, module);
  register_module__FILE__(vm, module);

  b_ptr_result result = interpret(vm, module, source);
  fflush(stdout);

  if (result == PTR_COMPILE_ERR)
    exit(EXIT_COMPILE);
  if (result == PTR_RUNTIME_ERR)
    exit(EXIT_RUNTIME);
}

void show_usage(char *argv[], bool fail) {
  FILE *out = fail ? stderr : stdout;
  fprintf(out, "Usage: %s [-[h | c | d | e | v | g | w]] [filename]\n", argv[0]);
  fprintf(out, "   -h       Show this help message.\n");
  fprintf(out, "   -v       Show version string.\n");
  fprintf(out, "   -b arg   Buffer terminal outputs with the given size.\n");
  fprintf(out, "   -d       Print bytecode.\n");
  fprintf(out, "   -e       Print bytecode and exit.\n");
  fprintf(out, "   -g arg   Sets the minimum heap size in kilobytes before the GC\n"
               "            can start. [Default = %d (%dmb)]\n", DEFAULT_GC_START / 1024,
          DEFAULT_GC_START / (1024 * 1024));
  fprintf(out, "   -c arg   Runs the give code.\n");
  fprintf(out, "   -w       Show runtime warnings.\n");
  exit(fail ? EXIT_FAILURE : EXIT_SUCCESS);
}

int main(int argc, char *argv[]) {

  bool show_warnings = false;
  bool should_print_bytecode = false;
  long stdout_buffer_size = 0L;
  bool should_exit_after_bytecode = false;
  char *source = NULL;
  int next_gc_start = DEFAULT_GC_START;

  if (argc > 1) {
    int opt;
    while ((opt = getopt(argc, argv, "hdeb:vg:wc:")) != -1) {
      switch (opt) {
        case 'h': {
          show_usage(argv, false);
          break;
        }// exits
        case 'd':
          should_print_bytecode = true;
          break;
        case 'e':
          should_print_bytecode = true;
          should_exit_after_bytecode = true;
          break;
        case 'b':
          stdout_buffer_size = strtol(optarg, NULL, 10);
          if (stdout_buffer_size < 0) {
            stdout_buffer_size = 0;
          }
          break;
        case 'v': {
          printf("Blade " BLADE_VERSION_STRING " (running on BladeVM " BVM_VERSION ")\n");
          return EXIT_SUCCESS;
        }
        case 'g': {
          int next = (int) strtol(optarg, NULL, 10);
          if (next > 0) {
            next_gc_start = next * 1024; // expected value is in kilobytes
          }
          break;
        }
        case 'c': {
          source = optarg;
          break;
        }
        case 'w': {
          show_warnings = true;
          break;
        }
        default: {
          show_usage(argv, true); // exits
          break;
        }
      }
    }
  }

  b_vm *vm = (b_vm *) malloc(sizeof(b_vm));
  if (vm != NULL) {
    memset(vm, 0, sizeof(b_vm));
    init_vm(vm);

    // set vm options...
    vm->show_warnings = show_warnings;
    vm->should_print_bytecode = should_print_bytecode;
    vm->should_exit_after_bytecode = should_exit_after_bytecode;
    vm->next_gc = next_gc_start;

    if (stdout_buffer_size) {
      // forcing printf buffering for TTYs and terminals
      if (isatty(fileno(stdout))) {
        char buffer[stdout_buffer_size];
        setvbuf(stdout, buffer, _IOFBF, stdout_buffer_size);
      }
    }

#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif

    char **std_args = (char**)calloc(argc, sizeof(char *));
    if(std_args != NULL) {
      for(int i = 0; i < argc; i++) {
        std_args[i] = strdup(argv[i]);
      }
      vm->std_args = std_args;
      vm->std_args_count = argc;
    }

    // always do this last so that we can have access to everything else
    bind_native_modules(vm);

    if (source != NULL) {
      run_code(vm, source);
    } else if (argc == 1 || argc <= optind) {
      repl(vm);
    } else {
      run_file(vm, argv[optind]);
    }

    free_vm(vm);
    free(std_args);
    return EXIT_SUCCESS;
  }

  fprintf(stderr, "Device out of memory.");
  exit(EXIT_FAILURE);
}
