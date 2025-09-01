#ifndef CORE_JSC_ERRORS_H
#define CORE_JSC_ERRORS_H

#include <JavaScriptCore/JavaScript.h>

typedef enum {
  ERR_NONE = 0,
  ERR_MEMORY,
  ERR_INVALID_ARGS,
  ERR_FILE_NOT_FOUND,
  ERR_FILE_IO,
  ERR_NETWORK,
  ERR_TIMEOUT,
  ERR_PERMISSION,
  ERR_SYNTAX
} error_code_t;

typedef struct {
  error_code_t code;
  char *message;
  char *stack_trace;
  JSValueRef js_error;
} runtime_error_t;

JSValueRef create_js_error(JSContextRef ctx, error_code_t code,
                           const char *message, const char *details);
void set_global_error_handler(JSContextRef ctx, JSObjectRef handler);
void emit_uncaught_exception(JSContextRef ctx, runtime_error_t *error);

#endif