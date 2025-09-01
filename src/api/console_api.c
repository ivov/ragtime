#include "api/console_api.h"
#include "constants.h"

#include <JavaScriptCore/JavaScript.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *concat_js_values_into_c_string(JSContextRef ctx, size_t argc,
                                            const JSValueRef args[],
                                            JSValueRef *js_err_str) {
  if (argc == 0) {
    return strdup("");
  }

  char **c_strs = malloc(argc * sizeof(char *));
  if (!c_strs) {
    return NULL;
  }
  size_t total_len = 0;

  for (size_t i = 0; i < argc; i++) {
    JSStringRef js_str = JSValueToStringCopy(ctx, args[i], js_err_str);
    if (*js_err_str) {
      for (size_t j = 0; j < i; j++) {
        free(c_strs[j]);
      }
      free(c_strs);
      return NULL;
    }

    size_t max_len = JSStringGetMaximumUTF8CStringSize(js_str);
    c_strs[i] = malloc(max_len);
    if (!c_strs[i]) {
      for (size_t j = 0; j < i; j++) {
        free(c_strs[j]);
      }
      free(c_strs);
      JSStringRelease(js_str);
      return NULL;
    }
    JSStringGetUTF8CString(js_str, c_strs[i], max_len);
    JSStringRelease(js_str);

    total_len += strlen(c_strs[i]) + 1; // +1 for space or null terminator
  }

  char *result = malloc(total_len);
  if (!result) {
    for (size_t i = 0; i < argc; i++) {
      free(c_strs[i]);
    }
    free(c_strs);
    return NULL;
  }
  result[0] = '\0';

  for (size_t i = 0; i < argc; i++) {
    strcat(result, c_strs[i]);
    if (i < argc - 1) {
      strcat(result, " ");
    }
    free(c_strs[i]);
  }

  free(c_strs);
  return result;
}

static JSValueRef console_output(const char *prefix, const char *ansi_color,
                                 JSContextRef ctx, JSObjectRef fn,
                                 JSObjectRef this_obj, size_t argc,
                                 const JSValueRef args[],
                                 JSValueRef *js_err_str) {
  if (argc == 0) {
    printf("%s%s: %s\n", ansi_color, prefix, ANSI_RESET);
    return JSValueMakeUndefined(ctx);
  }

  char *msg = concat_js_values_into_c_string(ctx, argc, args, js_err_str);
  if (msg) {
    printf("%s%s: %s%s\n", ansi_color, prefix, msg, ANSI_RESET);
    free(msg);
  }

  return JSValueMakeUndefined(ctx);
}

JSValueRef console_log(JSContextRef ctx, JSObjectRef fn, JSObjectRef this_obj,
                       size_t argc, const JSValueRef args[],
                       JSValueRef *js_err_str) {
  return console_output("LOG", ANSI_RESET, ctx, fn, this_obj, argc, args,
                        js_err_str);
}

JSValueRef console_warn(JSContextRef ctx, JSObjectRef fn, JSObjectRef this_obj,
                        size_t argc, const JSValueRef args[],
                        JSValueRef *js_err_str) {
  return console_output("WARN", ANSI_YELLOW, ctx, fn, this_obj, argc, args,
                        js_err_str);
}

JSValueRef console_info(JSContextRef ctx, JSObjectRef fn, JSObjectRef this_obj,
                        size_t argc, const JSValueRef args[],
                        JSValueRef *js_err_str) {
  return console_output("INFO", ANSI_BLUE, ctx, fn, this_obj, argc, args,
                        js_err_str);
}

JSValueRef console_debug(JSContextRef ctx, JSObjectRef fn, JSObjectRef this_obj,
                         size_t argc, const JSValueRef args[],
                         JSValueRef *js_err_str) {
  return console_output("DEBUG", ANSI_GRAY, ctx, fn, this_obj, argc, args,
                        js_err_str);
}

JSValueRef console_error(JSContextRef ctx, JSObjectRef function,
                         JSObjectRef this_obj, size_t argc,
                         const JSValueRef args[], JSValueRef *js_err_str) {
  return console_output("ERROR", ANSI_RED, ctx, function, this_obj, argc, args,
                        js_err_str);
}
