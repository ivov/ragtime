#include "core/jsc.h"

#include <JavaScriptCore/JavaScript.h>

JSGlobalContextRef create_js_context() {
  JSGlobalContextRef ctx = JSGlobalContextCreate(NULL);

  bind_native_apis(ctx);

  return ctx;
}

void execute_js(JSGlobalContextRef ctx, const char *js_script) {
  JSStringRef jsc_script = JSStringCreateWithUTF8CString(js_script);

  JSEvaluateScript(ctx, jsc_script, NULL, NULL, 1, NULL);

  JSStringRelease(jsc_script);
}

void cleanup_js_context(JSGlobalContextRef ctx) {
  JSGlobalContextRelease(ctx);
}
