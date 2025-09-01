#include "api/events_api.h"
#include "api/fs_api.h"
#include "api/module_api.h"
#include "cli.h"
#include "constants.h"
#include "core/jsc.h"
#include "core/libuv.h"

#include <stdio.h>
#include <stdlib.h>

void cleanup_js_context(JSGlobalContextRef ctx);

int process_argc;
char **process_argv;

void print_version() { printf("%s v%s\n", RUNTIME_NAME, RUNTIME_VERSION); }

void print_help() {
  printf("Usage: ragtime [options] [script.js]\n");
  printf("Options:\n");
  printf("  --version   Print version\n");
  printf("  --help      Show help\n");
  printf("  --eval <code> Execute inline code\n");
}

int main(int argc, char **argv) {
  process_argc = argc;
  process_argv = argv;

  parse_result result = parse_args(argc, argv);

  switch (result.cmd) {
  case CMD_VERSION:
    print_version();
    return EXIT_SUCCESS;

  case CMD_HELP:
    print_help();
    return EXIT_SUCCESS;

  case CMD_ERROR:
    fprintf(stderr, "Error: --eval requires code arg\n");
    return EXIT_FAILURE;

  case CMD_EVAL: {
    init_module_cache();
    JSGlobalContextRef ctx = create_js_context();
    init_events_api(ctx);
    init_event_loop();
    execute_js(ctx, result.arg);
    run_event_loop();
    clear_module_cache(ctx);
    cleanup_js_context(ctx);
    return EXIT_SUCCESS;
  }

  case CMD_RUN_FILE: {
    char *script = read_file(result.arg);
    if (!script) {
      return EXIT_FAILURE;
    }

    init_module_cache();
    JSGlobalContextRef ctx = create_js_context();
    init_events_api(ctx);
    init_event_loop();
    set_current_module_dir(result.arg);
    execute_js(ctx, script);
    run_event_loop();
    clear_module_cache(ctx);
    cleanup_js_context(ctx);
    free(script);
    return EXIT_SUCCESS;
  }

  case CMD_NONE:
    fprintf(stderr, "Error: No script or --eval provided\n");
    print_help();
    return EXIT_FAILURE;
  }

  return EXIT_FAILURE;
}
