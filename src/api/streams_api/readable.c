#include "api/streams_api.h"
#include "api/streams_api/queue.h"
#include "constants.h"
#include "core/jsc_interop.h"

#include <fcntl.h>
#include <stdlib.h>
#include <string.h>

static JSClassRef readable_stream_class = NULL;

static void on_stream_read(uv_fs_t *req);
static void on_stream_open_for_read(uv_fs_t *req);
static void schedule_next_read(ReadableStreamState *state);
static void drain_queue(ReadableStreamState *state);

static void readable_stream_finalize(JSObjectRef object) {
  ReadableStreamState *state = JSObjectGetPrivate(object);
  if (!state)
    return;

  if (state->data_handler)
    JSValueUnprotect(state->ctx, state->data_handler);
  if (state->end_handler)
    JSValueUnprotect(state->ctx, state->end_handler);
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

static void emit_stream_event(ReadableStreamState *state,
                              const char *event_name, JSValueRef data) {
  JSObjectRef handler = NULL;

  if (strcmp(event_name, "data") == 0)
    handler = state->data_handler;
  else if (strcmp(event_name, "end") == 0)
    handler = state->end_handler;
  else if (strcmp(event_name, "error") == 0)
    handler = state->error_handler;

  if (!handler)
    return;

  JSValueRef args[] = {data};
  JSValueRef exception = NULL;
  JSObjectCallAsFunction(state->ctx, handler, state->stream_obj, 1, args,
                         &exception);

  if (exception) {
    JSStringRef err_str = JSValueToStringCopy(state->ctx, exception, NULL);
    char err_buffer[ERROR_MSG_BUFFER_SIZE];
    JSStringGetUTF8CString(err_str, err_buffer, sizeof(err_buffer));
    fprintf(stderr, "Stream event handler error: %s\n", err_buffer);
    JSStringRelease(err_str);
  }
}

static void drain_queue(ReadableStreamState *state) {
  while (!stream_queue_is_empty(state->queue) && state->flowing &&
         state->data_handler) {
    StreamChunk *chunk = stream_queue_dequeue(state->queue);
    if (chunk) {
      JSStringRef chunk_str = JSStringCreateWithUTF8CString(chunk->data);
      JSValueRef data = JSValueMakeString(state->ctx, chunk_str);
      JSStringRelease(chunk_str);
      emit_stream_event(state, "data", data);
      free(chunk->data);
      free(chunk);
    }
  }
}

static void schedule_next_read(ReadableStreamState *state) {
  if (!state->flowing || state->ended || state->reading)
    return;

  state->reading = true;

  state->read_buffer =
      uv_buf_init(malloc(state->chunk_size), state->chunk_size);
  state->fs_req.data = state;

  uv_fs_read(uv_default_loop(), &state->fs_req, state->fd, &state->read_buffer,
             1, state->file_position, on_stream_read);
}

static void on_stream_read(uv_fs_t *req) {
  ReadableStreamState *state = req->data;
  state->reading = false;

  if (req->result < 0) {
    char err_msg[ERROR_MSG_BUFFER_SIZE];
    snprintf(err_msg, sizeof(err_msg), "Stream read error: %s",
             uv_strerror(req->result));
    JSStringRef err_str = JSStringCreateWithUTF8CString(err_msg);
    JSValueRef error = JSValueMakeString(state->ctx, err_str);
    JSStringRelease(err_str);
    emit_stream_event(state, "error", error);

    free(req->bufs[0].base);
    uv_fs_req_cleanup(req);
    return;
  }

  if (req->result == 0) {
    state->ended = true;
    emit_stream_event(state, "end", JSValueMakeUndefined(state->ctx));
    free(state->read_buffer.base);
    uv_fs_req_cleanup(req);
    return;
  }

  char *chunk_data = malloc(req->result + 1);
  memcpy(chunk_data, state->read_buffer.base, req->result);
  chunk_data[req->result] = '\0';

  state->file_position += req->result;

  if (state->flowing && state->data_handler) {
    // emit when flowing
    JSStringRef chunk_str = JSStringCreateWithUTF8CString(chunk_data);
    JSValueRef data = JSValueMakeString(state->ctx, chunk_str);
    JSStringRelease(chunk_str);
    emit_stream_event(state, "data", data);
    free(chunk_data);
  } else {
    // enqueue when paused
    stream_queue_enqueue(state->queue, chunk_data, req->result);
  }

  free(state->read_buffer.base);
  uv_fs_req_cleanup(req);

  schedule_next_read(state);
}

static void on_stream_open_for_read(uv_fs_t *req) {
  ReadableStreamState *state = req->data;

  if (req->result < 0) {
    char err_msg[ERROR_MSG_BUFFER_SIZE];
    snprintf(err_msg, sizeof(err_msg), "Cannot open file '%s': %s", state->path,
             uv_strerror(req->result));
    JSStringRef err_str = JSStringCreateWithUTF8CString(err_msg);
    JSValueRef error = JSValueMakeString(state->ctx, err_str);
    JSStringRelease(err_str);
    emit_stream_event(state, "error", error);
    uv_fs_req_cleanup(req);
    return;
  }

  state->fd = req->result;
  uv_fs_req_cleanup(req);

  if (state->flowing && state->data_handler) {
    schedule_next_read(state);
  }
}

JSValueRef fs_create_read_stream(JSContextRef ctx, JSObjectRef js_fn,
                                 JSObjectRef this_obj, size_t argc,
                                 const JSValueRef args[],
                                 JSValueRef *js_err_str) {
  if (!is_valid_argc(ctx, argc, 1, "fs.createReadStream", js_err_str)) {
    return JSValueMakeUndefined(ctx);
  }

  char *path = to_c_str(ctx, args[0], js_err_str);
  if (*js_err_str) {
    return JSValueMakeUndefined(ctx);
  }

  ReadableStreamState *state = calloc(1, sizeof(ReadableStreamState));
  state->ctx = ctx;
  state->path = strdup(path);
  state->chunk_size = STREAM_CHUNK_SIZE;
  state->high_watermark = STREAM_HIGH_WATERMARK;
  state->flowing = false;
  state->ended = false;
  state->reading = false;
  state->file_position = 0;
  state->queue = malloc(sizeof(StreamQueue));
  stream_queue_init(state->queue);

  if (readable_stream_class == NULL) {
    JSClassDefinition class_def = kJSClassDefinitionEmpty;
    class_def.finalize = readable_stream_finalize;
    readable_stream_class = JSClassCreate(&class_def);
  }

  JSObjectRef stream = JSObjectMake(ctx, readable_stream_class, state);
  state->stream_obj = stream;
  JSValueProtect(ctx, stream);

  JSStringRef on_name = JSStringCreateWithUTF8CString("on");
  JSObjectRef on_fn =
      JSObjectMakeFunctionWithCallback(ctx, on_name, readable_stream_on);
  JSObjectSetProperty(ctx, stream, on_name, on_fn, kJSPropertyAttributeNone,
                      NULL);
  JSStringRelease(on_name);

  JSStringRef pause_name = JSStringCreateWithUTF8CString("pause");
  JSObjectRef pause_fn =
      JSObjectMakeFunctionWithCallback(ctx, pause_name, readable_stream_pause);
  JSObjectSetProperty(ctx, stream, pause_name, pause_fn,
                      kJSPropertyAttributeNone, NULL);
  JSStringRelease(pause_name);

  JSStringRef resume_name = JSStringCreateWithUTF8CString("resume");
  JSObjectRef resume_fn = JSObjectMakeFunctionWithCallback(
      ctx, resume_name, readable_stream_resume);
  JSObjectSetProperty(ctx, stream, resume_name, resume_fn,
                      kJSPropertyAttributeNone, NULL);
  JSStringRelease(resume_name);

  JSStringRef pipe_name = JSStringCreateWithUTF8CString("pipe");
  JSObjectRef pipe_fn =
      JSObjectMakeFunctionWithCallback(ctx, pipe_name, readable_stream_pipe);
  JSObjectSetProperty(ctx, stream, pipe_name, pipe_fn, kJSPropertyAttributeNone,
                      NULL);
  JSStringRelease(pipe_name);

  state->fs_req.data = state; // back pointer for later access
  uv_fs_open(uv_default_loop(), &state->fs_req, path, O_RDONLY, 0,
             on_stream_open_for_read);

  free(path);
  return stream;
}

JSValueRef readable_stream_on(JSContextRef ctx, JSObjectRef js_fn,
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

  ReadableStreamState *state = JSObjectGetPrivate(this_obj);

  if (!state) {
    free(event_name);
    set_js_error(ctx, ERR_INVALID_STREAM_STATE, js_err_str);
    return JSValueMakeUndefined(ctx);
  }

  JSObjectRef handler = (JSObjectRef)args[1];
  JSValueProtect(ctx, handler);

  if (strcmp(event_name, "data") == 0) {
    if (state->data_handler) {
      JSValueUnprotect(ctx, state->data_handler);
    }
    state->data_handler = handler;
    state->flowing = true;
    if (state->fd > 0) {
      drain_queue(state);
      schedule_next_read(state);
    }
  } else if (strcmp(event_name, "end") == 0) {
    if (state->end_handler) {
      JSValueUnprotect(ctx, state->end_handler);
    }
    state->end_handler = handler;
  } else if (strcmp(event_name, "error") == 0) {
    if (state->error_handler) {
      JSValueUnprotect(ctx, state->error_handler);
    }
    state->error_handler = handler;
  }

  free(event_name);
  return this_obj;
}

JSValueRef readable_stream_pause(JSContextRef ctx, JSObjectRef js_fn,
                                 JSObjectRef this_obj, size_t argc,
                                 const JSValueRef args[],
                                 JSValueRef *js_err_str) {
  ReadableStreamState *state = JSObjectGetPrivate(this_obj);
  if (!state) {
    set_js_error(ctx, ERR_INVALID_STREAM_STATE, js_err_str);
    return JSValueMakeUndefined(ctx);
  }

  state->flowing = false;
  return this_obj;
}

JSValueRef readable_stream_resume(JSContextRef ctx, JSObjectRef js_fn,
                                  JSObjectRef this_obj, size_t argc,
                                  const JSValueRef args[],
                                  JSValueRef *js_err_str) {
  ReadableStreamState *state = JSObjectGetPrivate(this_obj);
  if (!state) {
    set_js_error(ctx, ERR_INVALID_STREAM_STATE, js_err_str);
    return JSValueMakeUndefined(ctx);
  }

  state->flowing = true;
  drain_queue(state);
  schedule_next_read(state);
  return this_obj;
}

JSValueRef readable_stream_pipe(JSContextRef ctx, JSObjectRef js_fn,
                                JSObjectRef this_obj, size_t argc,
                                const JSValueRef args[],
                                JSValueRef *js_err_str) {
  if (!is_valid_argc(ctx, argc, 1, "stream.pipe", js_err_str)) {
    return JSValueMakeUndefined(ctx);
  }

  if (!JSValueIsObject(ctx, args[0])) {
    set_js_error(ctx, "Pipe destination must be a writable stream", js_err_str);
    return JSValueMakeUndefined(ctx);
  }

  JSObjectRef dest = (JSObjectRef)args[0];

  ReadableStreamState *read_state = JSObjectGetPrivate(this_obj);
  if (!read_state) {
    set_js_error(ctx, ERR_INVALID_STREAM_STATE, js_err_str);
    return JSValueMakeUndefined(ctx);
  }

  const char *pipe_handler_code =
      "(function(readable, writable) {"
      "  readable.on('data', function(chunk) {"
      "    var canWriteMore = writable.write(chunk);"
      "    if (!canWriteMore) {"
      "      readable.pause();"
      "      writable.on('drain', function() {"
      "        readable.resume();"
      "      });"
      "    }"
      "  });"
      "  readable.on('end', function() {"
      "    writable.end();"
      "  });"
      "  readable.on('error', function(err) {"
      "    console.error('Pipe read error:', err);"
      "    writable.end();"
      "  });"
      "})";

  JSStringRef code_str = JSStringCreateWithUTF8CString(pipe_handler_code);
  JSValueRef exception = NULL;
  JSValueRef pipe_fn_val =
      JSEvaluateScript(ctx, code_str, NULL, NULL, 1, &exception);
  JSStringRelease(code_str);

  if (exception) {
    *js_err_str = exception;
    return JSValueMakeUndefined(ctx);
  }

  JSObjectRef pipe_fn = JSValueToObject(ctx, pipe_fn_val, &exception);
  if (exception) {
    *js_err_str = exception;
    return JSValueMakeUndefined(ctx);
  }

  JSValueRef pipe_args[] = {this_obj, dest};
  JSObjectCallAsFunction(ctx, pipe_fn, NULL, 2, pipe_args, &exception);

  if (exception) {
    *js_err_str = exception;
    return JSValueMakeUndefined(ctx);
  }

  return dest;
}
