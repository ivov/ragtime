#include "core/jsc_interop.h"

#include "constants.h"
#include "core/jsc_errors.h"

#include <JavaScriptCore/JavaScript.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <uv.h>

bool is_valid_argc(JSContextRef ctx, size_t argc, size_t expected_count,
                   const char *fn_name, JSValueRef *js_err_str) {
  if (argc >= expected_count) {
    return true;
  }

  char err_msg[ERROR_MSG_BUFFER_SIZE];
  snprintf(err_msg, sizeof(err_msg), "%s requires at least %zu argument%s",
           fn_name, expected_count, expected_count == 1 ? "" : "s");
  JSStringRef err_str = JSStringCreateWithUTF8CString(err_msg);
  *js_err_str = JSValueMakeString(ctx, err_str);
  JSStringRelease(err_str);
  return false;
}

bool to_callback(JSContextRef ctx, JSValueRef js_callback,
                 JSObjectRef *callback_out, JSValueRef *js_err_str) {
  if (!JSValueIsObject(ctx, js_callback)) {
    JSStringRef err_msg =
        JSStringCreateWithUTF8CString(ERR_CALLBACK_REQUIRED);
    *js_err_str = JSValueMakeString(ctx, err_msg);
    JSStringRelease(err_msg);
    return false;
  }

  *callback_out = JSValueToObject(ctx, js_callback, js_err_str);
  if (*js_err_str) {
    return false;
  }

  if (!JSObjectIsFunction(ctx, *callback_out)) {
    JSStringRef err_msg =
        JSStringCreateWithUTF8CString(ERR_CALLBACK_REQUIRED);
    *js_err_str = JSValueMakeString(ctx, err_msg);
    JSStringRelease(err_msg);
    return false;
  }

  return true;
}

char *to_c_str(JSContextRef ctx, JSValueRef js_value, JSValueRef *js_err_str) {
  JSStringRef js_str = JSValueToStringCopy(ctx, js_value, js_err_str);
  if (*js_err_str) {
    return NULL;
  }

  size_t length = JSStringGetMaximumUTF8CStringSize(js_str);
  char *c_string = malloc(length);
  if (!c_string) {
    JSStringRelease(js_str);
    JSStringRef err_msg =
        JSStringCreateWithUTF8CString(ERR_MEMORY_ALLOCATION);
    *js_err_str = JSValueMakeString(ctx, err_msg);
    JSStringRelease(err_msg);
    return NULL;
  }

  JSStringGetUTF8CString(js_str, c_string, length);
  JSStringRelease(js_str);
  return c_string;
}

void set_js_error(JSContextRef ctx, const char *message, JSValueRef *js_err_str) {
  JSStringRef msg = JSStringCreateWithUTF8CString(message);
  *js_err_str = JSValueMakeString(ctx, msg);
  JSStringRelease(msg);
}

void invoke_callback_with_err(JSContextRef ctx, JSObjectRef callback,
                              int uv_result, const char *op_name,
                              bool include_null_data) {
  error_code_t code;
  if (uv_result == UV_ENOENT) {
    code = ERR_FILE_NOT_FOUND;
  } else if (uv_result < 0) {
    code = ERR_FILE_IO;
  } else {
    code = ERR_NONE;
  }

  const char *msg = (uv_result < 0) ? uv_strerror(uv_result) : op_name;
  JSValueRef error_obj = create_js_error(ctx, code, op_name, msg);

  if (include_null_data) {
    // callback(error, null)
    JSValueRef args[] = {error_obj, JSValueMakeNull(ctx)};
    JSObjectCallAsFunction(ctx, callback, NULL, 2, args, NULL);
  } else {
    // callback(error)
    JSValueRef args[] = {error_obj};
    JSObjectCallAsFunction(ctx, callback, NULL, 1, args, NULL);
  }
}

void invoke_callback_with_custom_err(JSContextRef ctx, JSObjectRef callback,
                                     error_code_t code, const char *message,
                                     bool include_null_data) {
  JSValueRef error_obj = create_js_error(ctx, code, message, NULL);

  if (include_null_data) {
    JSValueRef args[] = {error_obj, JSValueMakeNull(ctx)};
    JSObjectCallAsFunction(ctx, callback, NULL, 2, args, NULL);
  } else {
    JSValueRef args[] = {error_obj};
    JSObjectCallAsFunction(ctx, callback, NULL, 1, args, NULL);
  }
}

bool validate_state(void *state, const char *op_name) {
  if (!state) {
    fprintf(stderr, "%s error: state is NULL\n", op_name);
    return false;
  }
  return true;
}
