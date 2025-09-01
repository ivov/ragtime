#ifndef API_CONSOLE_API_H
#define API_CONSOLE_API_H

#include <JavaScriptCore/JavaScript.h>

JSValueRef console_log(JSContextRef ctx, JSObjectRef js_fn,
                       JSObjectRef this_obj, size_t argc,
                       const JSValueRef args[], JSValueRef *js_err_str);

JSValueRef console_warn(JSContextRef ctx, JSObjectRef js_fn,
                        JSObjectRef this_obj, size_t argc,
                        const JSValueRef args[], JSValueRef *js_err_str);

JSValueRef console_info(JSContextRef ctx, JSObjectRef js_fn,
                        JSObjectRef this_obj, size_t argc,
                        const JSValueRef args[], JSValueRef *js_err_str);

JSValueRef console_debug(JSContextRef ctx, JSObjectRef js_fn,
                         JSObjectRef this_obj, size_t argc,
                         const JSValueRef args[], JSValueRef *js_err_str);

JSValueRef console_error(JSContextRef ctx, JSObjectRef js_fn,
                         JSObjectRef this_obj, size_t argc,
                         const JSValueRef args[], JSValueRef *js_err_str);

#endif
