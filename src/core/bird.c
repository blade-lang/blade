#include "util.h"
#include "vm.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#if !defined _WIN32 && !defined __CYGWIN__

#include <readline/history.h>
#include <readline/readline.h>

#endif

#include <setjmp.h>
#include <signal.h>

#ifdef _WIN32
#include "win32.h"
#endif

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
  int brace_count = 0, paren_count = 0, bracket_count = 0, single_quote_count = 0, double_quote_count = 0;

  for (;;) {
    while (sigsetjmp(ctrlc_buf, 1) != 0);

    if (!continue_repl) {
      current_line = 0;
      brace_count = 0;
      paren_count = 0;
      bracket_count = 0;
      single_quote_count = 0;
      double_quote_count = 0;

      // reset source...
      memset(source, 0, strlen(source));
      continue_repl = true;
    }

    current_line++;

    const char *cursor = "> ";
    if (brace_count > 0 || bracket_count > 0 || paren_count > 0) {
      cursor = "| ";
    } else if (single_quote_count == 1 || double_quote_count == 1) {
      cursor = "  ";
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

      // quotes
      else if(((i == 0 && line[i] == '\'') || (line[i] == '\'' && line[i - 1] != '\\')) && double_quote_count == 0) {
        if(single_quote_count == 0) 
          single_quote_count = 1; 
        else 
          single_quote_count = 0;
      }
      else if(((i == 0 && line[i] == '"') || (line[i] == '"' && line[i - 1] != '\\')) && single_quote_count == 0){
        if(double_quote_count == 0) 
          double_quote_count = 1; 
        else 
          double_quote_count = 0;
      }

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

    if (bracket_count == 0 && paren_count == 0 && brace_count == 0 && single_quote_count == 0 && double_quote_count == 0) {

      interpret(vm, source, "<repl>");

      // reset source...
      memset(source, 0, strlen(source));
    }
  }
}

static void run_file(b_vm *vm, const char *file) {
  char *source = read_file(file);
  if (source == NULL) {
    fprintf(stderr, "(Bird):\n  Launch aborted for %s\n  Reason: %s\n", file, strerror(errno));
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