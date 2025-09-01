#ifndef API_HTTP_API_H
#define API_HTTP_API_H

#include <JavaScriptCore/JavaScript.h>

JSValueRef http_get(JSContextRef ctx, JSObjectRef js_fn, JSObjectRef this_obj,
                    size_t argc, const JSValueRef args[],
                    JSValueRef *js_err_str);

JSValueRef http_create_server(JSContextRef ctx, JSObjectRef js_fn,
                              JSObjectRef this_obj, size_t argc,
                              const JSValueRef args[], JSValueRef *js_err_str);

JSValueRef http_server_listen(JSContextRef ctx, JSObjectRef js_fn,
                              JSObjectRef this_obj, size_t argc,
                              const JSValueRef args[], JSValueRef *js_err_str);

#endif
