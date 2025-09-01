#include "cli.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

parse_result parse_args(int argc, char **argv) {
  parse_result result = {CMD_NONE, NULL};

  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "--version") == 0) {
      result.cmd = CMD_VERSION;
      return result;
    }

    if (strcmp(argv[i], "--help") == 0) {
      result.cmd = CMD_HELP;
      return result;
    }

    if (strcmp(argv[i], "--eval") == 0) {
      if (i + 1 >= argc) {
        result.cmd = CMD_ERROR;
        return result;
      }
      result.cmd = CMD_EVAL;
      result.arg = argv[i + 1];
      return result;
    }

    // arg is not an option, must be a filename
    result.cmd = CMD_RUN_FILE;
    result.arg = argv[i];
    return result;
  }

  return result;
}

char *read_file(const char *filename) {
  FILE *f = fopen(filename, "rb");
  if (!f) {
    fprintf(stderr, "Error: Could not open file %s\n", filename);
    return NULL;
  }

  fseek(f, 0, SEEK_END);
  long len = ftell(f);
  rewind(f);

  char *content = malloc(len + 1);
  if (!content) {
    fprintf(stderr, "Error: Failed to allocate memory for script\n");
    fclose(f);
    return NULL;
  }

  size_t bytes_read = fread(content, 1, len, f);
  if (bytes_read != (size_t)len) {
    fprintf(stderr,
            "Error: Failed to read file (expected %ld bytes, got %zu)\n", len,
            bytes_read);
    free(content);
    fclose(f);
    return NULL;
  }

  content[len] = '\0';
  fclose(f);
  return content;
}
