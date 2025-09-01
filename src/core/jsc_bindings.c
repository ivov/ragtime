#include "core/jsc.h"

#include "api/console_api.h"
#include "api/fs_api.h"
#include "api/http_api.h"
#include "api/module_api.h"
#include "api/net_api.h"
#include "api/process_api.h"
#include "api/streams_api.h"
#include "api/timer_api.h"

#include <JavaScriptCore/JavaScript.h>

extern int process_argc;
extern char **process_argv;

extern JSValueRef http_get(JSContextRef ctx, JSObjectRef function,
                           JSObjectRef this_obj, size_t argc,
                           const JSValueRef args[], JSValueRef *exception);

extern JSValueRef http_create_server(JSContextRef ctx, JSObjectRef function,
                                     JSObjectRef this_obj, size_t argc,
                                     const JSValueRef args[],
                                     JSValueRef *exception);

static void bind_fn(JSContextRef ctx, JSObjectRef parent, const char *name,
                    JSObjectCallAsFunctionCallback callback) {
  JSStringRef jsName = JSStringCreateWithUTF8CString(name);
  JSObjectSetProperty(ctx, parent, jsName,
                      JSObjectMakeFunctionWithCallback(ctx, jsName, callback),
                      kJSPropertyAttributeNone, NULL);
  JSStringRelease(jsName);
}

static JSObjectRef create_and_bind_object(JSContextRef ctx,
                                          JSObjectRef parent_obj,
                                          const char *name) {
  JSObjectRef obj = JSObjectMake(ctx, NULL, NULL);
  JSStringRef jsName = JSStringCreateWithUTF8CString(name);
  JSObjectSetProperty(ctx, parent_obj, jsName, obj, kJSPropertyAttributeNone,
                      NULL);
  JSStringRelease(jsName);
  return obj;
}

static void bind_console_api(JSGlobalContextRef ctx, JSObjectRef global) {
  static const struct {
    const char *name;
    JSObjectCallAsFunctionCallback callback;
  } console_fns[] = {{"log", console_log},
                     {"warn", console_warn},
                     {"info", console_info},
                     {"debug", console_debug},
                     {"error", console_error}};

  JSObjectRef console = create_and_bind_object(ctx, global, "console");

  const size_t console_count = sizeof(console_fns) / sizeof(console_fns[0]);
  for (size_t i = 0; i < console_count; i++) {
    bind_fn(ctx, console, console_fns[i].name, console_fns[i].callback);
  }
}

static void bind_timer_api(JSGlobalContextRef ctx, JSObjectRef global) {
  static const struct {
    const char *name;
    JSObjectCallAsFunctionCallback callback;
  } timer_fns[] = {{"setTimeout", js_set_timeout},
                   {"clearTimeout", js_clear_timeout},
                   {"setInterval", js_set_interval},
                   {"clearInterval", js_clear_interval}};

  const size_t timer_count = sizeof(timer_fns) / sizeof(timer_fns[0]);
  for (size_t i = 0; i < timer_count; i++) {
    bind_fn(ctx, global, timer_fns[i].name, timer_fns[i].callback);
  }
}

static void bind_process_api(JSGlobalContextRef ctx, JSObjectRef global) {
  JSObjectRef process = create_and_bind_object(ctx, global, "process");

  JSObjectRef argv = JSObjectMakeArray(ctx, 0, NULL, NULL);
  for (int i = 0; i < process_argc; i++) {
    JSStringRef arg = JSStringCreateWithUTF8CString(process_argv[i]);
    JSObjectSetPropertyAtIndex(ctx, argv, i, JSValueMakeString(ctx, arg), NULL);
    JSStringRelease(arg);
  }
  JSStringRef argvName = JSStringCreateWithUTF8CString("argv");
  JSObjectSetProperty(ctx, process, argvName, argv, kJSPropertyAttributeNone,
                      NULL);
  JSStringRelease(argvName);

  bind_fn(ctx, process, "exit", js_process_exit);
  bind_fn(ctx, process, "on", js_process_on);
  
  JSValueRef env_obj = js_process_env(ctx, NULL, NULL, 0, NULL, NULL);
  JSStringRef envName = JSStringCreateWithUTF8CString("env");
  JSObjectSetProperty(ctx, process, envName, env_obj, kJSPropertyAttributeReadOnly,
                      NULL);
  JSStringRelease(envName);
}

static void bind_fs_api(JSGlobalContextRef ctx, JSObjectRef global) {
  static const struct {
    const char *name;
    JSObjectCallAsFunctionCallback callback;
  } fs_fns[] = {{"readFile", fs_read_file},
                {"writeFile", fs_write_file},
                {"exists", fs_exists},
                {"readFileAsync", fs_read_file_async},
                {"createReadStream", fs_create_read_stream},
                {"createWriteStream", fs_create_write_stream}};

  JSObjectRef fs = create_and_bind_object(ctx, global, "fs");

  const size_t fs_count = sizeof(fs_fns) / sizeof(fs_fns[0]);
  for (size_t i = 0; i < fs_count; i++) {
    bind_fn(ctx, fs, fs_fns[i].name, fs_fns[i].callback);
  }
}

static void bind_http_api(JSGlobalContextRef ctx, JSObjectRef global) {
  static const struct {
    const char *name;
    JSObjectCallAsFunctionCallback callback;
  } http_functions[] = {{"get", http_get},
                        {"createServer", http_create_server}};

  JSObjectRef http = create_and_bind_object(ctx, global, "http");

  const size_t http_count = sizeof(http_functions) / sizeof(http_functions[0]);
  for (size_t i = 0; i < http_count; i++) {
    bind_fn(ctx, http, http_functions[i].name, http_functions[i].callback);
  }
}

static void bind_net_api(JSGlobalContextRef ctx, JSObjectRef global) {
  static const struct {
    const char *name;
    JSObjectCallAsFunctionCallback callback;
  } net_functions[] = {{"createServer", net_create_server}};

  JSObjectRef net = create_and_bind_object(ctx, global, "net");

  const size_t net_count = sizeof(net_functions) / sizeof(net_functions[0]);
  for (size_t i = 0; i < net_count; i++) {
    bind_fn(ctx, net, net_functions[i].name, net_functions[i].callback);
  }
}

void bind_native_apis(JSGlobalContextRef ctx) {
  JSObjectRef global = JSContextGetGlobalObject(ctx);

  bind_console_api(ctx, global);
  bind_timer_api(ctx, global);
  bind_process_api(ctx, global);
  bind_fs_api(ctx, global);
  bind_http_api(ctx, global);
  bind_net_api(ctx, global);

  bind_fn(ctx, global, "require", js_require);
}
