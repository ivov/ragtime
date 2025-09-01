#ifndef API_FS_API_H
#define API_FS_API_H

#include <JavaScriptCore/JavaScript.h>

JSValueRef fs_read_file(JSContextRef ctx, JSObjectRef js_fn,
                        JSObjectRef this_obj, size_t argc,
                        const JSValueRef args[], JSValueRef *js_err_str);

JSValueRef fs_write_file(JSContextRef ctx, JSObjectRef js_fn,
                         JSObjectRef this_obj, size_t argc,
                         const JSValueRef args[], JSValueRef *js_err_str);

JSValueRef fs_exists(JSContextRef ctx, JSObjectRef js_fn, JSObjectRef this_obj,
                     size_t argc, const JSValueRef args[],
                     JSValueRef *js_err_str);

JSValueRef fs_read_file_async(JSContextRef ctx, JSObjectRef js_fn,
                              JSObjectRef this_obj, size_t argc,
                              const JSValueRef args[], JSValueRef *js_err_str);

char *read_file(const char *filename);

#endif
