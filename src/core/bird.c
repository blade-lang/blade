#include "vm.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void repl(b_vm *vm) {
  vm->is_repl = true;
  char line[4096];

  for (;;) {
    printf("$ ");

    if (!fgets(line, sizeof(line), stdin)) {
      printf("\n");
      break;
    }

    interpret(vm, line, "<repl>");
  }
}

static char *read_file(const char *path) {
  FILE *fp = fopen(path, "rb");

  // file not readable (maybe due to permission)
  if (fp == NULL) {
    fprintf(stderr, "could not open file %s\n", path);
    exit(74);
  }

  fseek(fp, 0L, SEEK_END);
  size_t file_size = ftell(fp);
  rewind(fp);

  char *buffer = (char *)malloc(file_size + 1);

  // the system might not have enough memory to read the file.
  if (buffer == NULL) {
    fprintf(stderr, "not enough memory to read file %s\n", path);
    exit(74);
  }

  size_t bytes_read = fread(buffer, sizeof(char), file_size, fp);

  // if we couldn't read the entire file
  if (bytes_read < file_size) {
    fprintf(stderr, "could not read file %s\n", path);
    exit(74);
  }

  buffer[bytes_read] = '\0';

  fclose(fp);
  return buffer;
}

static void run_file(b_vm *vm, const char *file) {
  char *source = read_file(file);
  b_ptr_result result = interpret(vm, source, file);
  free(source);

  if (result == PTR_COMPILE_ERR)
    exit(65);
  if (result == PTR_RUNTIME_ERR)
    exit(70);
}

int main(int argc, const char *argv[]) {
  b_vm vm;
  init_vm(&vm);

  if (argc == 1) {
    repl(&vm);
  } else if (argc == 2) {
    run_file(&vm, argv[1]);
  } else {
    fprintf(stderr, "Usage: bird [path]\n");
    exit(64);
  }

  free_vm(&vm);
  return 0;
}