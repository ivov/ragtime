#ifndef CLI_H
#define CLI_H

typedef enum {
  CMD_NONE,
  CMD_VERSION,
  CMD_HELP,
  CMD_EVAL,
  CMD_RUN_FILE,
  CMD_ERROR
} command_type;

typedef struct {
  command_type cmd;
  char *arg;
} parse_result;

parse_result parse_args(int argc, char **argv);

char *read_file(const char *filename);

#endif
