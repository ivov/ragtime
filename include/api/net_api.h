#ifndef API_NET_API_H
#define API_NET_API_H

#include <JavaScriptCore/JavaScript.h>

JSValueRef net_create_server(JSContextRef ctx, JSObjectRef js_fn,
                             JSObjectRef this_obj, size_t argc,
                             const JSValueRef args[], JSValueRef *js_err_str);

JSValueRef net_server_listen(JSContextRef ctx, JSObjectRef js_fn,
                             JSObjectRef this_obj, size_t argc,
                             const JSValueRef args[], JSValueRef *js_err_str);

JSValueRef client_write(JSContextRef ctx, JSObjectRef js_fn,
                        JSObjectRef this_obj, size_t argc,
                        const JSValueRef args[], JSValueRef *js_err_str);

#endif
