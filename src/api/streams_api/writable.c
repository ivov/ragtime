#include "api/streams_api.h"
#include "api/streams_api/queue.h"
#include "constants.h"
#include "core/jsc_interop.h"

#include <fcntl.h>
#include <stdlib.h>
#include <string.h>

static JSClassRef writable_stream_class = NULL;

static void on_stream_write(uv_fs_t *req);
static void on_stream_open_for_write(uv_fs_t *req);
static void process_write_queue(WritableStreamState *state);

static void writable_stream_finalize(JSObjectRef object) {
  WritableStreamState *state = JSObjectGetPrivate(object);
  if (!state)
    return;

  if (state->drain_handler)
    JSValueUnprotect(state->ctx, state->drain_handler);
  if (state->finish_handler)
    JSValueUnprotect(state->ctx, state->finish_handler);
  if (state->error_handler)
    JSValueUnprotect(state->ctx, state->error_handler);

  if (state->queue) {
    stream_queue_free(state->queue);
    free(state->queue);
  }

  if (state->fd > 0) {
    uv_fs_t close_req;
    uv_fs_close(uv_default_loop(), &close_req, state->fd, NULL);
    uv_fs_req_cleanup(&close_req);
  }

  free(state->path);
  free(state);
}

static void emit_writable_event(WritableStreamState *state,
                                const char *event_name) {
  JSObjectRef handler = NULL;

  if (strcmp(event_name, "drain") == 0)
    handler = state->drain_handler;
  else if (strcmp(event_name, "finish") == 0)
    handler = state->finish_handler;
  else if (strcmp(event_name, "error") == 0)
    handler = state->error_handler;

  if (!handler)
    return;

  JSValueRef exception = NULL;
  JSObjectCallAsFunction(state->ctx, handler, state->stream_obj, 0, NULL,
                         &exception);

  if (exception) {
    JSStringRef err_str = JSValueToStringCopy(state->ctx, exception, NULL);
    char err_buffer[ERROR_MSG_BUFFER_SIZE];
    JSStringGetUTF8CString(err_str, err_buffer, sizeof(err_buffer));
    fprintf(stderr, "Stream event handler error: %s\n", err_buffer);
    JSStringRelease(err_str);
  }
}

static void process_write_queue(WritableStreamState *state) {
  if (state->writing || stream_queue_is_empty(state->queue) || state->fd <= 0) {
    return;
  }

  state->writing = true;
  StreamChunk *chunk = state->queue->head;

  state->write_buffer = uv_buf_init(chunk->data, chunk->length);
  state->fs_req.data = state; // back pointer for later access

  uv_fs_write(uv_default_loop(), &state->fs_req, state->fd,
              &state->write_buffer, 1, -1, on_stream_write);
}

static void on_stream_write(uv_fs_t *req) {
  WritableStreamState *state = req->data;
  state->writing = false;

  if (req->result < 0) {
    char err_msg[ERROR_MSG_BUFFER_SIZE];
    snprintf(err_msg, sizeof(err_msg), "Stream write error: %s",
             uv_strerror(req->result));
    fprintf(stderr, "%s\n", err_msg);
    uv_fs_req_cleanup(req);
    return;
  }

  StreamChunk *written = stream_queue_dequeue(state->queue);
  if (written) {
    free(written->data);
    free(written);
  }

  uv_fs_req_cleanup(req);

  if (state->needs_drain && state->queue->total_size < state->high_watermark) {
    state->needs_drain = false;
    emit_writable_event(state, "drain");
  }

  process_write_queue(state);

  if (state->ended && stream_queue_is_empty(state->queue)) {
    emit_writable_event(state, "finish");
  }
}

static void on_stream_open_for_write(uv_fs_t *req) {
  WritableStreamState *state = req->data;

  if (req->result < 0) {
    char err_msg[ERROR_MSG_BUFFER_SIZE];
    snprintf(err_msg, sizeof(err_msg), "Cannot open file '%s' for writing: %s",
             state->path, uv_strerror(req->result));
    fprintf(stderr, "%s\n", err_msg);
    uv_fs_req_cleanup(req);
    return;
  }

  state->fd = req->result;
  uv_fs_req_cleanup(req);

  process_write_queue(state);
}

JSValueRef fs_create_write_stream(JSContextRef ctx, JSObjectRef js_fn,
                                  JSObjectRef this_obj, size_t argc,
                                  const JSValueRef args[],
                                  JSValueRef *js_err_str) {
  if (!is_valid_argc(ctx, argc, 1, "fs.createWriteStream", js_err_str)) {
    return JSValueMakeUndefined(ctx);
  }

  char *path = to_c_str(ctx, args[0], js_err_str);
  if (*js_err_str) {
    return JSValueMakeUndefined(ctx);
  }

  WritableStreamState *state = calloc(1, sizeof(WritableStreamState));
  state->ctx = ctx;
  state->path = strdup(path);
  state->high_watermark = STREAM_HIGH_WATERMARK;
  state->needs_drain = false;
  state->ended = false;
  state->writing = false;
  state->fd = 0; // will be set when file opens
  state->queue = malloc(sizeof(StreamQueue));
  stream_queue_init(state->queue);

  if (writable_stream_class == NULL) {
    JSClassDefinition class_def = kJSClassDefinitionEmpty;
    class_def.finalize = writable_stream_finalize;
    writable_stream_class = JSClassCreate(&class_def);
  }

  JSObjectRef stream = JSObjectMake(ctx, writable_stream_class, state);
  state->stream_obj = stream;
  JSValueProtect(ctx, stream);

  JSStringRef write_name = JSStringCreateWithUTF8CString("write");
  JSObjectRef write_fn =
      JSObjectMakeFunctionWithCallback(ctx, write_name, writable_stream_write);
  JSObjectSetProperty(ctx, stream, write_name, write_fn,
                      kJSPropertyAttributeNone, NULL);
  JSStringRelease(write_name);

  JSStringRef end_name = JSStringCreateWithUTF8CString("end");
  JSObjectRef end_fn =
      JSObjectMakeFunctionWithCallback(ctx, end_name, writable_stream_end);
  JSObjectSetProperty(ctx, stream, end_name, end_fn, kJSPropertyAttributeNone,
                      NULL);
  JSStringRelease(end_name);

  JSStringRef on_name = JSStringCreateWithUTF8CString("on");
  JSObjectRef on_fn =
      JSObjectMakeFunctionWithCallback(ctx, on_name, writable_stream_on);
  JSObjectSetProperty(ctx, stream, on_name, on_fn, kJSPropertyAttributeNone,
                      NULL);
  JSStringRelease(on_name);

  state->fs_req.data = state;
  uv_fs_open(uv_default_loop(), &state->fs_req, path,
             O_WRONLY | O_CREAT | O_TRUNC, FILE_DEFAULT_PERMISSIONS,
             on_stream_open_for_write);

  free(path);
  return stream;
}

JSValueRef writable_stream_write(JSContextRef ctx, JSObjectRef js_fn,
                                 JSObjectRef this_obj, size_t argc,
                                 const JSValueRef args[],
                                 JSValueRef *js_err_str) {
  if (!is_valid_argc(ctx, argc, 1, "stream.write", js_err_str)) {
    return JSValueMakeUndefined(ctx);
  }

  char *data = to_c_str(ctx, args[0], js_err_str);
  if (*js_err_str) {
    return JSValueMakeUndefined(ctx);
  }

  WritableStreamState *state = JSObjectGetPrivate(this_obj);
  if (!state) {
    free(data);
    set_js_error(ctx, ERR_INVALID_STREAM_STATE, js_err_str);
    return JSValueMakeUndefined(ctx);
  }

  if (state->ended) {
    free(data);
    set_js_error(ctx, "Cannot write after end", js_err_str);
    return JSValueMakeUndefined(ctx);
  }

  size_t length = strlen(data);
  stream_queue_enqueue(state->queue, data, length);

  if (!state->writing) {
    process_write_queue(state);
  }

  bool can_continue = state->queue->total_size < state->high_watermark;
  if (!can_continue) {
    state->needs_drain = true;
  }

  return JSValueMakeBoolean(ctx, can_continue);
}

JSValueRef writable_stream_end(JSContextRef ctx, JSObjectRef js_fn,
                               JSObjectRef this_obj, size_t argc,
                               const JSValueRef args[],
                               JSValueRef *js_err_str) {
  WritableStreamState *state = JSObjectGetPrivate(this_obj);
  if (!state) {
    set_js_error(ctx, ERR_INVALID_STREAM_STATE, js_err_str);
    return JSValueMakeUndefined(ctx);
  }

  state->ended = true;

  if (stream_queue_is_empty(state->queue)) {
    emit_writable_event(state, "finish");
  }

  return JSValueMakeUndefined(ctx);
}

JSValueRef writable_stream_on(JSContextRef ctx, JSObjectRef js_fn,
                              JSObjectRef this_obj, size_t argc,
                              const JSValueRef args[], JSValueRef *js_err_str) {
  if (!is_valid_argc(ctx, argc, 2, "stream.on", js_err_str)) {
    return JSValueMakeUndefined(ctx);
  }

  char *event_name = to_c_str(ctx, args[0], js_err_str);
  if (*js_err_str) {
    return JSValueMakeUndefined(ctx);
  }

  if (!JSValueIsObject(ctx, args[1]) ||
      !JSObjectIsFunction(ctx, (JSObjectRef)args[1])) {
    free(event_name);
    set_js_error(ctx, ERR_CALLBACK_REQUIRED, js_err_str);
    return JSValueMakeUndefined(ctx);
  }

  WritableStreamState *state = JSObjectGetPrivate(this_obj);
  if (!state) {
    free(event_name);
    set_js_error(ctx, ERR_INVALID_STREAM_STATE, js_err_str);
    return JSValueMakeUndefined(ctx);
  }

  JSObjectRef handler = (JSObjectRef)args[1];
  JSValueProtect(ctx, handler);

  if (strcmp(event_name, "drain") == 0) {
    if (state->drain_handler) {
      JSValueUnprotect(ctx, state->drain_handler);
    }
    state->drain_handler = handler;
  } else if (strcmp(event_name, "finish") == 0) {
    if (state->finish_handler) {
      JSValueUnprotect(ctx, state->finish_handler);
    }
    state->finish_handler = handler;
  } else if (strcmp(event_name, "error") == 0) {
    if (state->error_handler) {
      JSValueUnprotect(ctx, state->error_handler);
    }
    state->error_handler = handler;
  }

  free(event_name);
  return this_obj;
}