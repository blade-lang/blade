#include "util.h"
#include "vm.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if !defined _WIN32 && !defined __CYGWIN__

#include <readline/history.h>
#include <readline/readline.h>

#endif

#include <setjmp.h>
#include <signal.h>

#include "win32.h"

static bool continue_repl = true;
sigjmp_buf ctrlc_buf;

void handle_signals(int sig_no) {
  if (sig_no == SIGINT) {
    printf("\n<KeyboardInterrupt>\n");
    continue_repl = false;
    siglongjmp(ctrlc_buf, 1);
  }
}

static void repl(b_vm *vm) {
  if (signal(SIGINT, handle_signals) == SIG_ERR) {
    printf("failed to register interrupts with kernel\n");
  }

  vm->is_repl = true;

  fprintf(stdout, "Bird %s (running on BVM %s), REPL/Interactive mode = ON\n",
          BIRD_VERSION_STRING, BVM_VERSION);
  fprintf(stdout, "%s, (Build time = %s, %s)\n", COMPILER, __DATE__, __TIME__);
  fprintf(stdout,
          "Type \"exit()\" to quit, \"help()\" or \"credits()\" for more "
          "information\n");

  char *source = (char *) calloc(1, sizeof(char));
  int current_line = 0;
  int brace_count = 0, paren_count = 0, bracket_count = 0;

  for (;;) {
    while (sigsetjmp(ctrlc_buf, 1) != 0);

    if (!continue_repl) {
      current_line = 0;
      brace_count = 0;
      paren_count = 0;
      bracket_count = 0;

      // reset source...
      memset(source, 0, strlen(source));
      continue_repl = true;
    }

    current_line++;

    const char *cursor = "> ";
    if (brace_count > 0 || bracket_count > 0 || paren_count > 0) {
      cursor = "| ";
    }

#if defined _WIN32 || defined __CYGWIN__
    char buffer[1024];
    printf(cursor);
    char *line = fgets(buffer, 1024, stdin);
    int line_length = strcspn(line, "\r\n");
    line[line_length] = 0;
#else
    char *line = readline(cursor);
    int line_length = (int) strlen(line);
#endif // _WIN32

    // terminate early if we receive a terminating command such as exit()

#if defined _MSC_VER && defined _DEBUG
    if (strcmp(line, "exit()", line_length) == 0) {
#else
    if (strcmp(line, "exit()") == 0) {
#endif
      exit(EXIT_SUCCESS);
    }

#if !defined _WIN32 && !defined __CYGWIN__
    // allow user to navigate through past input in terminal...
    add_history(line);
#endif // !_WIN32

    // find count of { and }, ( and ), [ and ]
    for (int i = 0; i < line_length; i++) {
      // scope openers...
      if (line[i] == '{')
        brace_count++;
      else if (line[i] == '(')
        paren_count++;
      else if (line[i] == '[')
        bracket_count++;

        // scope closers...
      else if (line[i] == '}' && brace_count > 0)
        brace_count--;
      else if (line[i] == ')' && paren_count > 0)
        paren_count--;
      else if (line[i] == ']' && bracket_count > 0)
        bracket_count--;
    }

    source = append_strings(source, line);
    if (line_length > 0) {
      source = append_strings(source, "\n");
    }

    if (bracket_count == 0 && paren_count == 0 && brace_count == 0) {

      interpret(vm, source, "<repl>");

      // reset source...
      memset(source, 0, strlen(source));
    }
  }
}

static char *read_file(const char *path) {
  FILE *fp = fopen(path, "rb");

  // file not readable (maybe due to permission)
  if (fp == NULL) {
    fprintf(stderr, "could not open file %s\n", path);
    return NULL;
  }

  fseek(fp, 0L, SEEK_END);
  size_t file_size = ftell(fp);
  rewind(fp);

  char *buffer = (char *) malloc(file_size + 1);

  // the system might not have enough memory to read the file.
  if (buffer == NULL) {
    fprintf(stderr, "not enough memory to read file %s\n", path);
    return NULL;
  }

  size_t bytes_read = fread(buffer, sizeof(char), file_size, fp);

  // if we couldn't read the entire file
  if (bytes_read < file_size) {
    fprintf(stderr, "could not read file %s\n", path);
    return NULL;
  }

  buffer[bytes_read] = '\0';

  fclose(fp);
  return buffer;
}

static void run_file(b_vm *vm, const char *file) {
  char *source = read_file(file);
  if (source == NULL) {
    exit(74);
  }

  b_ptr_result result = interpret(vm, source, file);
  free(source);

  if (result == PTR_COMPILE_ERR)
    exit(65);
  if (result == PTR_RUNTIME_ERR)
    exit(70);
}

int main(int argc, const char *argv[]) {
  b_vm *vm = (b_vm *) malloc(sizeof(b_vm));
  memset(vm, 0, sizeof(b_vm));

  init_vm(vm);

  if (argc == 1) {
    repl(vm);
  } else if (argc == 2) {
    run_file(vm, argv[1]);
  } else {
    fprintf(stderr, "Usage: bird [path]\n");
    exit(64);
  }

  free_vm(vm);
  return 0;
}