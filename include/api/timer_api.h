#ifndef API_TIMER_API_H
#define API_TIMER_API_H

#include <JavaScriptCore/JavaScript.h>
#include <stdint.h>

extern uint32_t next_timer_id;

JSValueRef js_set_timeout(JSContextRef ctx, JSObjectRef js_fn,
                          JSObjectRef this_obj, size_t argc,
                          const JSValueRef args[], JSValueRef *js_err_str);

JSValueRef js_clear_timeout(JSContextRef ctx, JSObjectRef js_fn,
                            JSObjectRef this_obj, size_t argc,
                            const JSValueRef args[], JSValueRef *js_err_str);

JSValueRef js_set_interval(JSContextRef ctx, JSObjectRef js_fn,
                           JSObjectRef this_obj, size_t argc,
                           const JSValueRef args[], JSValueRef *js_err_str);

JSValueRef js_clear_interval(JSContextRef ctx, JSObjectRef js_fn,
                             JSObjectRef this_obj, size_t argc,
                             const JSValueRef args[], JSValueRef *js_err_str);

#endif
