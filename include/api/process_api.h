#ifndef API_PROCESS_API_H
#define API_PROCESS_API_H

#include <JavaScriptCore/JavaScript.h>

JSValueRef js_process_exit(JSContextRef ctx, JSObjectRef js_fn,
                           JSObjectRef this_obj, size_t argc,
                           const JSValueRef args[], JSValueRef *js_err_str);

JSValueRef js_process_on(JSContextRef ctx, JSObjectRef js_fn,
                         JSObjectRef this_obj, size_t argc,
                         const JSValueRef args[], JSValueRef *js_err_str);

JSValueRef js_process_env(JSContextRef ctx, JSObjectRef js_fn,
                          JSObjectRef this_obj, size_t argc,
                          const JSValueRef args[], JSValueRef *js_err_str);

#endif
