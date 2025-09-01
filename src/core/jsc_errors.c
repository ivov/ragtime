#include "core/jsc_errors.h"
#include "core/jsc_interop.h"

#include <JavaScriptCore/JavaScript.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static JSObjectRef global_error_handler = NULL;
static JSContextRef error_ctx = NULL;

static const char *error_code_names[] = {
    "None",         "MemoryError",     "InvalidArguments",
    "FileNotFound", "FileIOError",     "NetworkError",
    "TimeoutError", "PermissionError", "SyntaxError"};

JSValueRef create_js_error(JSContextRef ctx, error_code_t code,
                           const char *message, const char *details) {
  JSObjectRef error_obj = JSObjectMake(ctx, NULL, NULL);

  JSStringRef code_prop = JSStringCreateWithUTF8CString("code");
  JSStringRef code_str = JSStringCreateWithUTF8CString(error_code_names[code]);
  JSObjectSetProperty(ctx, error_obj, code_prop,
                      JSValueMakeString(ctx, code_str),
                      kJSPropertyAttributeNone, NULL);
  JSStringRelease(code_prop);
  JSStringRelease(code_str);

  JSStringRef msg_prop = JSStringCreateWithUTF8CString("message");
  JSStringRef msg_str = JSStringCreateWithUTF8CString(message);
  JSObjectSetProperty(ctx, error_obj, msg_prop, JSValueMakeString(ctx, msg_str),
                      kJSPropertyAttributeNone, NULL);
  JSStringRelease(msg_prop);
  JSStringRelease(msg_str);

  if (details) {
    JSStringRef details_prop = JSStringCreateWithUTF8CString("details");
    JSStringRef details_str = JSStringCreateWithUTF8CString(details);
    JSObjectSetProperty(ctx, error_obj, details_prop,
                        JSValueMakeString(ctx, details_str),
                        kJSPropertyAttributeNone, NULL);
    JSStringRelease(details_prop);
    JSStringRelease(details_str);
  }

  return error_obj;
}

void set_global_error_handler(JSContextRef ctx, JSObjectRef handler) {
  if (global_error_handler && error_ctx) {
    JSValueUnprotect(error_ctx, global_error_handler);
  }
  global_error_handler = handler;
  error_ctx = ctx;
  JSValueProtect(ctx, handler);
}

void emit_uncaught_exception(JSContextRef ctx, runtime_error_t *error) {
  if (global_error_handler) {
    JSValueRef args[] = {error->js_error};
    JSObjectCallAsFunction(ctx, global_error_handler, NULL, 1, args, NULL);
    return;
  }

  fprintf(stderr, "Uncaught %s: %s\n", error_code_names[error->code],
          error->message);
  exit(1);
}

