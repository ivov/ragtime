#include "api/module_api.h"
#include "api/fs_api.h"
#include "core/jsc_interop.h"
#include "hashtable.h"

#include <JavaScriptCore/JavaScript.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static HashTable module_cache;
static char *current_module_dir = NULL; // dir of currently executing script

void init_module_cache(void) { hashtable_init(&module_cache); }

void clear_module_cache(JSContextRef ctx) {
  for (int i = 0; i < HASHTABLE_SIZE; i++) {
    HashEntry *entry = module_cache.slots[i];
    while (entry) {
      HashEntry *next = entry->next;

      if (entry->value) {
        JSValueUnprotect(ctx, (JSObjectRef)entry->value);
      }

      free(entry->key);
      free(entry);
      entry = next;
    }
    module_cache.slots[i] = NULL;
  }

  if (current_module_dir) {
    free(current_module_dir);
    current_module_dir = NULL;
  }
}

JSObjectRef get_cached_module(const char *module_path) {
  return (JSObjectRef)hashtable_get(&module_cache, module_path);
}

void cache_module(JSContextRef ctx, const char *module_path,
                  JSObjectRef exports) {
  JSValueProtect(ctx, exports);
  hashtable_put(&module_cache, module_path, exports);
}

void set_current_module_dir(const char *script_path) {
  if (current_module_dir) {
    free(current_module_dir);
  }

  char *path_copy = strdup(script_path);
  if (!path_copy)
    return;

  char *dir = dirname(path_copy);
  current_module_dir = strdup(dir);
  free(path_copy);

  if (!current_module_dir) {
    fprintf(stderr, "Failed to allocate memory for module directory\n");
  }
}

static char *resolve_module_path(const char *user_path) {
  if (user_path[0] == '/') {
    return realpath(user_path, NULL);
  }

  if (strncmp(user_path, "./", 2) != 0) {
    return NULL; // bare module names not supported (typically node_modules)
  }

  if (!current_module_dir) {
    return NULL;
  }

  const char *relative_path = user_path + 2; // skip "./"
  size_t total_len = strlen(current_module_dir) + strlen(relative_path) + 2;
  char *resolved_path = malloc(total_len);

  if (!resolved_path) {
    return NULL;
  }

  snprintf(resolved_path, total_len, "%s/%s", current_module_dir,
           relative_path);
  char *final_resolved_path = realpath(resolved_path, NULL);
  free(resolved_path);

  return final_resolved_path;
}

static JSObjectRef load_module(JSContextRef ctx, const char *module_path,
                               JSValueRef *js_err_str) {
  char *file_content = read_file(module_path);
  if (!file_content) {
    char err_msg[ERROR_MSG_BUFFER_SIZE];
    snprintf(err_msg, sizeof(err_msg), "Cannot find module '%s'", module_path);
    set_js_error(ctx, err_msg, js_err_str);
    return NULL;
  }

  char *saved_module_dir =
      current_module_dir ? strdup(current_module_dir) : NULL;

  set_current_module_dir(module_path);

  JSObjectRef exports = JSObjectMake(ctx, NULL, NULL);
  JSObjectRef global = JSContextGetGlobalObject(ctx);
  JSStringRef module_name = JSStringCreateWithUTF8CString("module");
  JSObjectRef module_obj = JSObjectMake(ctx, NULL, NULL);
  JSStringRef exports_name = JSStringCreateWithUTF8CString("exports");
  JSObjectSetProperty(ctx, module_obj, exports_name, exports,
                      kJSPropertyAttributeNone, NULL);
  JSObjectSetProperty(ctx, global, module_name, module_obj,
                      kJSPropertyAttributeNone, NULL);
  JSStringRef jsc_script = JSStringCreateWithUTF8CString(file_content);
  JSValueRef exception = NULL;

  JSEvaluateScript(ctx, jsc_script, NULL, NULL, 1, &exception);

  if (current_module_dir) {
    free(current_module_dir);
  }
  current_module_dir = saved_module_dir;

  if (exception) {
    *js_err_str = exception;
    JSStringRelease(jsc_script);
    JSStringRelease(module_name);
    JSStringRelease(exports_name);
    free(file_content);
    return NULL;
  }

  JSValueRef final_exports =
      JSObjectGetProperty(ctx, module_obj, exports_name, NULL);
  JSObjectRef final_exports_obj = JSValueToObject(ctx, final_exports, NULL);

  JSValueRef undefined = JSValueMakeUndefined(ctx);
  JSObjectSetProperty(ctx, global, module_name, undefined,
                      kJSPropertyAttributeNone, NULL);

  JSStringRelease(jsc_script);
  JSStringRelease(module_name);
  JSStringRelease(exports_name);
  free(file_content);

  return final_exports_obj;
}

JSValueRef js_require(JSContextRef ctx, JSObjectRef js_fn, JSObjectRef this_obj,
                      size_t argc, const JSValueRef args[],
                      JSValueRef *js_err_str) {
  if (!is_valid_argc(ctx, argc, 1, "require", js_err_str)) {
    return JSValueMakeUndefined(ctx);
  }

  char *module_path = to_c_str(ctx, args[0], js_err_str);
  if (*js_err_str) {
    return JSValueMakeUndefined(ctx);
  }

  char *resolved_module_path = resolve_module_path(module_path);
  if (!resolved_module_path) {
    free(module_path);
    JSStringRef err_msg =
        JSStringCreateWithUTF8CString("Failed to resolve module path");
    *js_err_str = JSValueMakeString(ctx, err_msg);
    JSStringRelease(err_msg);
    return JSValueMakeUndefined(ctx);
  }

  JSObjectRef cached_exports = get_cached_module(resolved_module_path);
  if (cached_exports) {
    free(module_path);
    free(resolved_module_path);
    return cached_exports;
  }

  JSObjectRef exports = load_module(ctx, resolved_module_path, js_err_str);
  if (*js_err_str || !exports) {
    free(module_path);
    free(resolved_module_path);
    return JSValueMakeUndefined(ctx);
  }

  cache_module(ctx, resolved_module_path, exports);

  free(module_path);
  free(resolved_module_path);

  return exports;
}
