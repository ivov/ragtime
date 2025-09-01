#ifndef CORE_JSC_PROMISE_H
#define CORE_JSC_PROMISE_H

#include <JavaScriptCore/JavaScript.h>

JSValueRef create_promise(JSContextRef ctx, JSObjectRef *resolve_out,
                          JSObjectRef *reject_out, JSValueRef *js_err_str);
void promise_resolve(JSContextRef ctx, JSObjectRef resolve_fn,
                     JSValueRef value);
void promise_reject(JSContextRef ctx, JSObjectRef reject_fn, JSValueRef error);

#endif