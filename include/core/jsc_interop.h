#ifndef CORE_JSC_INTEROP_H
#define CORE_JSC_INTEROP_H

#include "jsc_errors.h"
#include <JavaScriptCore/JavaScript.h>
#include <stdbool.h>
#include <uv.h>

bool is_valid_argc(JSContextRef ctx, size_t argc, size_t expected_count,
                   const char *fn_name, JSValueRef *js_err_str);
bool to_callback(JSContextRef ctx, JSValueRef js_callback,
                 JSObjectRef *callback_out, JSValueRef *js_err_str);
char *to_c_str(JSContextRef ctx, JSValueRef js_value, JSValueRef *js_err_str);
void set_js_error(JSContextRef ctx, const char *message,
                  JSValueRef *js_err_str);

void invoke_callback_with_err(JSContextRef ctx, JSObjectRef callback,
                              int uv_result, const char *op_name,
                              bool include_null_data);
void invoke_callback_with_custom_err(JSContextRef ctx, JSObjectRef callback,
                                     error_code_t code, const char *message,
                                     bool include_null_data);

bool validate_state(void *state, const char *op_name);

#endif
