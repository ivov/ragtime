#include "core/jsc_promise.h"

#include <JavaScriptCore/JavaScript.h>

JSValueRef create_promise(JSContextRef ctx, JSObjectRef *resolve_out,
                          JSObjectRef *reject_out, JSValueRef *js_err_str) {
  JSStringRef code = JSStringCreateWithUTF8CString(
      "(function() {"
      "  var res, rej;"
      "  var p = new Promise(function(resolve, reject) {"
      "    res = resolve; rej = reject;"
      "  });"
      "  return [p, res, rej];"
      "})()");

  JSValueRef result = JSEvaluateScript(ctx, code, NULL, NULL, 0, js_err_str);
  JSStringRelease(code);

  if (*js_err_str) {
    return JSValueMakeUndefined(ctx);
  }

  JSObjectRef result_array = JSValueToObject(ctx, result, js_err_str);
  if (*js_err_str) {
    return JSValueMakeUndefined(ctx);
  }

  JSValueRef promise =
      JSObjectGetPropertyAtIndex(ctx, result_array, 0, js_err_str);
  if (*js_err_str) {
    return JSValueMakeUndefined(ctx);
  }

  *resolve_out =
      (JSObjectRef)JSObjectGetPropertyAtIndex(ctx, result_array, 1, js_err_str);
  if (*js_err_str) {
    return JSValueMakeUndefined(ctx);
  }

  *reject_out =
      (JSObjectRef)JSObjectGetPropertyAtIndex(ctx, result_array, 2, js_err_str);
  if (*js_err_str) {
    return JSValueMakeUndefined(ctx);
  }

  return promise;
}

void promise_resolve(JSContextRef ctx, JSObjectRef resolve_fn,
                     JSValueRef value) {
  if (resolve_fn && JSObjectIsFunction(ctx, resolve_fn)) {
    JSValueRef args[] = {value};
    JSObjectCallAsFunction(ctx, resolve_fn, NULL, 1, args, NULL);
  }
}

void promise_reject(JSContextRef ctx, JSObjectRef reject_fn, JSValueRef error) {
  if (reject_fn && JSObjectIsFunction(ctx, reject_fn)) {
    JSValueRef args[] = {error};
    JSObjectCallAsFunction(ctx, reject_fn, NULL, 1, args, NULL);
  }
}