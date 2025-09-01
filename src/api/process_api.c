#include "api/process_api.h"
#include "core/jsc_errors.h"
#include "core/jsc_interop.h"
#include "core/libuv.h"

#include <JavaScriptCore/JavaScript.h>
#include <stdlib.h>
#include <string.h>

extern int process_argc;
extern char **process_argv;
extern char **environ;

JSValueRef js_process_exit(JSContextRef ctx, JSObjectRef function,
                           JSObjectRef this_obj, size_t argc,
                           const JSValueRef args[], JSValueRef *exception) {
  int code = 0;
  if (argc > 0) {
    code = (int)JSValueToNumber(ctx, args[0], exception);
    if (*exception)
      return JSValueMakeUndefined(ctx);
  }

  uv_stop(loop);
  exit(code);
}

JSValueRef js_process_env(JSContextRef ctx, JSObjectRef function,
                          JSObjectRef this_obj, size_t argc,
                          const JSValueRef args[], JSValueRef *exception) {
  JSObjectRef env_obj = JSObjectMake(ctx, NULL, NULL);
  
  for (char **env = environ; *env != NULL; env++) {
    char *env_var = *env;
    char *equals = strchr(env_var, '=');
    if (!equals) continue;
    
    size_t key_len = equals - env_var;
    char *key = malloc(key_len + 1);
    if (!key) continue;
    
    strncpy(key, env_var, key_len);
    key[key_len] = '\0';
    
    char *value = equals + 1;
    
    JSStringRef js_key = JSStringCreateWithUTF8CString(key);
    JSStringRef js_value = JSStringCreateWithUTF8CString(value);
    JSValueRef js_value_ref = JSValueMakeString(ctx, js_value);
    
    JSObjectSetProperty(ctx, env_obj, js_key, js_value_ref, 
                        kJSPropertyAttributeNone, NULL);
    
    JSStringRelease(js_key);
    JSStringRelease(js_value);
    free(key);
  }
  
  return env_obj;
}

JSValueRef js_process_on(JSContextRef ctx, JSObjectRef js_fn,
                         JSObjectRef this_obj, size_t argc,
                         const JSValueRef args[], JSValueRef *js_err_str) {
  if (!is_valid_argc(ctx, argc, 2, "process.on", js_err_str)) {
    return JSValueMakeUndefined(ctx);
  }

  char *event_name = to_c_str(ctx, args[0], js_err_str);
  if (*js_err_str) {
    return JSValueMakeUndefined(ctx);
  }

  if (strcmp(event_name, "uncaughtException") == 0) {
    JSObjectRef handler;
    if (to_callback(ctx, args[1], &handler, js_err_str)) {
      set_global_error_handler(ctx, handler);
    }
  }

  free(event_name);

  return JSValueMakeUndefined(ctx);
}
