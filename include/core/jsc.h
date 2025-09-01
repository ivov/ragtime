#ifndef CORE_JSC_H
#define CORE_JSC_H

#include <JavaScriptCore/JavaScript.h>
#include <stdbool.h>

// exec
JSGlobalContextRef create_js_context();
void execute_js(JSGlobalContextRef ctx, const char *script);

// API binding
void bind_native_apis(JSGlobalContextRef ctx);

#endif