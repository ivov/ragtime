#ifndef API_STREAMS_API_H
#define API_STREAMS_API_H

#include <JavaScriptCore/JavaScript.h>
#include <stdbool.h>
#include <uv.h>

#define STREAM_CHUNK_SIZE 4096
#define STREAM_HIGH_WATERMARK 16384

// Queue structure is defined in api/streams_api/queue.h
struct StreamQueue;

typedef struct {
  uv_fs_t fs_req;
  uv_file fd;
  JSContextRef ctx;
  JSObjectRef stream_obj;
  JSObjectRef data_handler;
  JSObjectRef end_handler;
  JSObjectRef error_handler;

  struct StreamQueue *queue;
  size_t high_watermark;
  size_t chunk_size;

  bool flowing;
  bool ended;
  bool reading;
  off_t file_position;
  char *path; // for error messages
  
  uv_buf_t read_buffer; // for current read operation
} ReadableStreamState;

typedef struct {
  uv_fs_t fs_req;
  uv_file fd;
  JSContextRef ctx;
  JSObjectRef stream_obj;
  JSObjectRef drain_handler;
  JSObjectRef finish_handler;
  JSObjectRef error_handler;

  struct StreamQueue *queue;
  size_t high_watermark;

  bool needs_drain;
  bool ended;
  bool writing;
  char *path; // for error messages
  
  uv_buf_t write_buffer; // for current write operation
} WritableStreamState;

JSValueRef fs_create_read_stream(JSContextRef ctx, JSObjectRef js_fn,
                                  JSObjectRef this_obj, size_t argc,
                                  const JSValueRef args[],
                                  JSValueRef *js_err_str);

JSValueRef fs_create_write_stream(JSContextRef ctx, JSObjectRef js_fn,
                                   JSObjectRef this_obj, size_t argc,
                                   const JSValueRef args[],
                                   JSValueRef *js_err_str);

JSValueRef readable_stream_on(JSContextRef ctx, JSObjectRef js_fn,
                               JSObjectRef this_obj, size_t argc,
                               const JSValueRef args[], JSValueRef *js_err_str);

JSValueRef readable_stream_pause(JSContextRef ctx, JSObjectRef js_fn,
                                  JSObjectRef this_obj, size_t argc,
                                  const JSValueRef args[],
                                  JSValueRef *js_err_str);

JSValueRef readable_stream_resume(JSContextRef ctx, JSObjectRef js_fn,
                                   JSObjectRef this_obj, size_t argc,
                                   const JSValueRef args[],
                                   JSValueRef *js_err_str);

JSValueRef readable_stream_pipe(JSContextRef ctx, JSObjectRef js_fn,
                                 JSObjectRef this_obj, size_t argc,
                                 const JSValueRef args[],
                                 JSValueRef *js_err_str);

JSValueRef writable_stream_write(JSContextRef ctx, JSObjectRef js_fn,
                                  JSObjectRef this_obj, size_t argc,
                                  const JSValueRef args[],
                                  JSValueRef *js_err_str);

JSValueRef writable_stream_end(JSContextRef ctx, JSObjectRef js_fn,
                                JSObjectRef this_obj, size_t argc,
                                const JSValueRef args[], JSValueRef *js_err_str);

JSValueRef writable_stream_on(JSContextRef ctx, JSObjectRef js_fn,
                               JSObjectRef this_obj, size_t argc,
                               const JSValueRef args[], JSValueRef *js_err_str);

#endif