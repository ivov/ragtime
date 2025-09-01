#ifndef API_MODULE_API_H
#define API_MODULE_API_H

#include "constants.h"
#include <JavaScriptCore/JavaScript.h>

typedef struct ModuleCacheEntry {
  char *path;
  JSObjectRef exports;
  struct ModuleCacheEntry *next; // for collision chaining
} ModuleCacheEntry;

typedef struct {
  ModuleCacheEntry *heads[MODULE_CACHE_SIZE];
} ModuleCache;

JSValueRef js_require(JSContextRef ctx, JSObjectRef js_fn, JSObjectRef this_obj,
                      size_t argc, const JSValueRef args[],
                      JSValueRef *js_err_str);

void init_module_cache(void);
void clear_module_cache(JSContextRef ctx);
JSObjectRef get_cached_module(const char *path);
void cache_module(JSContextRef ctx, const char *path, JSObjectRef exports);
void set_current_module_dir(const char *script_path);

#endif
