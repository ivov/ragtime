#include "api/fs_api.h"
#include "constants.h"
#include "core/jsc_errors.h"
#include "core/jsc_interop.h"
#include "core/jsc_promise.h"

#include <JavaScriptCore/JavaScript.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <uv.h>

typedef struct {
  uv_fs_t req;
  uv_buf_t buffer;
  JSContextRef ctx;
  JSObjectRef callback;
} FileOpState;

static void on_file_read(uv_fs_t *req) {
  FileOpState *state = (FileOpState *)req->data;
  uv_file fd = req->file;

  if (!validate_state(state, "fs.readFile")) {
    uv_fs_req_cleanup(req);
    return;
  }

  if (req->result < 0) {
    invoke_callback_with_err(state->ctx, state->callback, req->result,
                             "fs.readFile", true);
    uv_fs_req_cleanup(req);
    uv_fs_close(uv_default_loop(), &state->req, fd, NULL);
    JSValueUnprotect(state->ctx, state->callback);
    free(state->buffer.base);
    free(state);
    return;
  }

  if (req->result == MAX_FILE_SIZE) {
    char err_msg[ERROR_MSG_BUFFER_SIZE];
    snprintf(err_msg, sizeof(err_msg),
             "File too large for read buffer (exceeds %d bytes)",
             MAX_FILE_SIZE);
    invoke_callback_with_custom_err(state->ctx, state->callback, ERR_FILE_IO,
                                    err_msg, true);
    uv_fs_req_cleanup(req);
    uv_fs_close(uv_default_loop(), &state->req, fd, NULL);
    JSValueUnprotect(state->ctx, state->callback);
    free(state->buffer.base);
    free(state);
    return;
  }

  state->buffer.base[req->result] = '\0';
  uv_fs_req_cleanup(req);
  JSStringRef file_content = JSStringCreateWithUTF8CString(state->buffer.base);
  JSValueRef args[] = {JSValueMakeNull(state->ctx),
                       JSValueMakeString(state->ctx, file_content)};
  JSObjectCallAsFunction(state->ctx, state->callback, NULL, 2, args, NULL);
  JSStringRelease(file_content);

  uv_fs_close(uv_default_loop(), &state->req, fd, NULL);

  JSValueUnprotect(state->ctx, state->callback);
  free(state->buffer.base);
  free(state);
}

static void on_file_open_for_read(uv_fs_t *req) {
  FileOpState *state = (FileOpState *)req->data;

  if (!validate_state(state, "fs.readFile")) {
    uv_fs_req_cleanup(req);
    return;
  }

  if (req->result < 0) {
    invoke_callback_with_err(state->ctx, state->callback, req->result,
                             "fs.readFile", true);
    uv_fs_req_cleanup(req);
    JSValueUnprotect(state->ctx, state->callback);
    free(state);
    return;
  }

  uv_file fd = req->result;
  uv_fs_req_cleanup(req);

  state->buffer = uv_buf_init((char *)malloc(MAX_FILE_SIZE), MAX_FILE_SIZE);
  if (!state->buffer.base) {
    fprintf(stderr, "Memory allocation failed\n");
    uv_fs_close(uv_default_loop(), &state->req, fd, NULL);
    JSValueUnprotect(state->ctx, state->callback);
    free(state);
    return;
  }

  uv_fs_read(uv_default_loop(), &state->req, fd, &state->buffer, 1, 0,
             on_file_read);
}

JSValueRef fs_read_file(JSContextRef ctx, JSObjectRef js_fn,
                        JSObjectRef this_obj, size_t argc,
                        const JSValueRef args[], JSValueRef *js_err_str) {
  if (!is_valid_argc(ctx, argc, 2, "fs.readFile", js_err_str)) {
    return JSValueMakeUndefined(ctx);
  }

  char *path = to_c_str(ctx, args[0], js_err_str);
  if (*js_err_str) {
    return JSValueMakeUndefined(ctx);
  }

  JSObjectRef callback;
  if (!to_callback(ctx, args[1], &callback, js_err_str)) {
    free(path);
    return JSValueMakeUndefined(ctx);
  }

  FileOpState *state = (FileOpState *)malloc(sizeof(FileOpState));
  if (!state) {
    free(path);
    set_js_error(ctx, ERR_MEMORY_ALLOCATION, js_err_str);
    return JSValueMakeUndefined(ctx);
  }

  state->ctx = ctx;
  state->callback = callback;
  JSValueProtect(ctx, state->callback);
  state->req.data = state; // back pointer for later access

  uv_fs_open(uv_default_loop(), &state->req, path, O_RDONLY, 0,
             on_file_open_for_read);
  free(path);

  return JSValueMakeUndefined(ctx);
}

void on_file_write(uv_fs_t *req) {
  FileOpState *state = (FileOpState *)req->data;
  uv_file fd = req->file;

  if (!validate_state(state, "fs.writeFile")) {
    uv_fs_req_cleanup(req);
    return;
  }

  if (req->result < 0) {
    invoke_callback_with_err(state->ctx, state->callback, req->result,
                             "fs.writeFile", false);
    uv_fs_req_cleanup(req);
    uv_fs_close(uv_default_loop(), &state->req, fd, NULL);
    JSValueUnprotect(state->ctx, state->callback);
    free(state->buffer.base);
    free(state);
    return;
  }

  uv_fs_req_cleanup(req);
  JSValueRef args[] = {JSValueMakeNull(state->ctx)};
  JSObjectCallAsFunction(state->ctx, state->callback, NULL, 1, args, NULL);

  uv_fs_close(uv_default_loop(), &state->req, fd, NULL);
  JSValueUnprotect(state->ctx, state->callback);
  free(state->buffer.base);
  free(state);
}

void on_file_open_for_write(uv_fs_t *req) {
  FileOpState *state = (FileOpState *)req->data;

  if (!validate_state(state, "fs.writeFile")) {
    uv_fs_req_cleanup(req);
    return;
  }

  if (req->result < 0) {
    invoke_callback_with_err(state->ctx, state->callback, req->result,
                             "fs.writeFile", false);
    uv_fs_req_cleanup(req);
    JSValueUnprotect(state->ctx, state->callback);
    free(state);
    return;
  }

  uv_file fd = req->result;
  uv_fs_req_cleanup(req);

  uv_fs_write(uv_default_loop(), &state->req, fd, &state->buffer, 1, 0,
              on_file_write);
}

JSValueRef fs_write_file(JSContextRef ctx, JSObjectRef js_fn,
                         JSObjectRef this_obj, size_t argc,
                         const JSValueRef args[], JSValueRef *js_err_str) {
  if (!is_valid_argc(ctx, argc, 3, "fs.writeFile", js_err_str)) {
    return JSValueMakeUndefined(ctx);
  }

  char *path = to_c_str(ctx, args[0], js_err_str);
  if (*js_err_str) {
    return JSValueMakeUndefined(ctx);
  }

  char *content = to_c_str(ctx, args[1], js_err_str);
  if (*js_err_str) {
    free(path);
    return JSValueMakeUndefined(ctx);
  }

  JSObjectRef callback;
  if (!to_callback(ctx, args[2], &callback, js_err_str)) {
    free(path);
    free(content);
    return JSValueMakeUndefined(ctx);
  }

  FileOpState *state = (FileOpState *)malloc(sizeof(FileOpState));
  if (!state) {
    free(path);
    free(content);
    set_js_error(ctx, ERR_MEMORY_ALLOCATION, js_err_str);
    return JSValueMakeUndefined(ctx);
  }

  state->ctx = ctx;
  state->callback = callback;
  JSValueProtect(ctx, state->callback);
  state->buffer = uv_buf_init(content, strlen(content));
  state->req.data = state; // back pointer for later access

  uv_fs_open(uv_default_loop(), &state->req, path, O_WRONLY | O_CREAT | O_TRUNC,
             FILE_DEFAULT_PERMISSIONS, on_file_open_for_write);
  free(path);

  return JSValueMakeUndefined(ctx);
}

void on_file_stat(uv_fs_t *req) {
  FileOpState *state = (FileOpState *)req->data;
  uv_fs_req_cleanup(req);

  if (!validate_state(state, "fs.exists")) {
    return;
  }

  JSValueRef args[2];

  if (req->result < 0) {
    args[0] = JSValueMakeNull(state->ctx);
    args[1] = JSValueMakeBoolean(state->ctx, false);
  } else {
    args[0] = JSValueMakeNull(state->ctx);
    args[1] = JSValueMakeBoolean(state->ctx, true);
  }

  JSObjectCallAsFunction(state->ctx, state->callback, NULL, 2, args, NULL);
  JSValueUnprotect(state->ctx, state->callback);
  free(state);
}

JSValueRef fs_exists(JSContextRef ctx, JSObjectRef js_fn, JSObjectRef this_obj,
                     size_t argc, const JSValueRef args[],
                     JSValueRef *js_err_str) {
  if (!is_valid_argc(ctx, argc, 2, "fs.exists", js_err_str)) {
    return JSValueMakeUndefined(ctx);
  }

  char *path = to_c_str(ctx, args[0], js_err_str);
  if (*js_err_str) {
    return JSValueMakeUndefined(ctx);
  }

  JSObjectRef callback;
  if (!to_callback(ctx, args[1], &callback, js_err_str)) {
    free(path);
    return JSValueMakeUndefined(ctx);
  }

  FileOpState *state = (FileOpState *)malloc(sizeof(FileOpState));
  if (!state) {
    free(path);
    set_js_error(ctx, ERR_MEMORY_ALLOCATION, js_err_str);
    return JSValueMakeUndefined(ctx);
  }

  state->ctx = ctx;
  state->callback = callback;
  JSValueProtect(ctx, state->callback);
  state->req.data = state; // back pointer for later access

  uv_fs_stat(uv_default_loop(), &state->req, path, on_file_stat);
  free(path);

  return JSValueMakeUndefined(ctx);
}

typedef struct {
  uv_fs_t req;
  JSContextRef ctx;
  JSObjectRef resolve;
  JSObjectRef reject;
  char *buffer;
} PromiseFileOp;

static void promise_read_callback(uv_fs_t *req) {
  PromiseFileOp *op = (PromiseFileOp *)req->data;
  if (!op) {
    uv_fs_req_cleanup(req);
    return;
  }
  
  uv_file fd = req->file;

  if (req->result < 0) {
    JSValueRef error = create_js_error(op->ctx, ERR_FILE_IO, "fs.readFileAsync",
                                       uv_strerror(req->result));
    promise_reject(op->ctx, op->reject, error);
  } else {
    op->buffer[req->result] = '\0';
    JSStringRef content = JSStringCreateWithUTF8CString(op->buffer);
    JSValueRef value = JSValueMakeString(op->ctx, content);
    promise_resolve(op->ctx, op->resolve, value);
    JSStringRelease(content);
  }

  uv_fs_req_cleanup(req);
  uv_fs_close(uv_default_loop(), &op->req, fd, NULL);
  JSValueUnprotect(op->ctx, op->resolve);
  JSValueUnprotect(op->ctx, op->reject);
  free(op->buffer);
  free(op);
}

static void promise_open_callback(uv_fs_t *req) {
  PromiseFileOp *op = (PromiseFileOp *)req->data;
  if (!op) {
    uv_fs_req_cleanup(req);
    return;
  }

  if (req->result < 0) {
    JSValueRef error =
        create_js_error(op->ctx, ERR_FILE_NOT_FOUND, "fs.readFileAsync",
                        uv_strerror(req->result));
    promise_reject(op->ctx, op->reject, error);
    uv_fs_req_cleanup(req);
    JSValueUnprotect(op->ctx, op->resolve);
    JSValueUnprotect(op->ctx, op->reject);
    free(op);
    return;
  }

  uv_file fd = req->result;
  uv_fs_req_cleanup(req);

  op->buffer = malloc(MAX_FILE_SIZE);
  if (!op->buffer) {
    JSValueRef error = create_js_error(op->ctx, ERR_MEMORY, "fs.readFileAsync",
                                       ERR_MEMORY_ALLOCATION);
    promise_reject(op->ctx, op->reject, error);
    uv_fs_close(uv_default_loop(), &op->req, fd, NULL);
    JSValueUnprotect(op->ctx, op->resolve);
    JSValueUnprotect(op->ctx, op->reject);
    free(op);
    return;
  }

  uv_buf_t buffer = uv_buf_init(op->buffer, MAX_FILE_SIZE - 1);
  uv_fs_read(uv_default_loop(), &op->req, fd, &buffer, 1, 0,
             promise_read_callback);
}

JSValueRef fs_read_file_async(JSContextRef ctx, JSObjectRef js_fn,
                              JSObjectRef this_obj, size_t argc,
                              const JSValueRef args[], JSValueRef *js_err_str) {
  if (!is_valid_argc(ctx, argc, 1, "fs.readFileAsync", js_err_str)) {
    return JSValueMakeUndefined(ctx);
  }

  char *filepath = to_c_str(ctx, args[0], js_err_str);
  if (*js_err_str) {
    return JSValueMakeUndefined(ctx);
  }

  JSObjectRef resolve, reject;
  JSValueRef promise = create_promise(ctx, &resolve, &reject, js_err_str);
  if (*js_err_str) {
    free(filepath);
    return JSValueMakeUndefined(ctx);
  }

  PromiseFileOp *op = malloc(sizeof(PromiseFileOp));
  if (!op) {
    free(filepath);
    set_js_error(ctx, ERR_MEMORY_ALLOCATION, js_err_str);
    return JSValueMakeUndefined(ctx);
  }

  op->ctx = ctx;
  op->resolve = resolve;
  op->reject = reject;
  op->buffer = NULL;
  op->req.data = op; // back pointer for later access

  JSValueProtect(ctx, resolve);
  JSValueProtect(ctx, reject);

  uv_fs_open(uv_default_loop(), &op->req, filepath, O_RDONLY, 0,
             promise_open_callback);
  free(filepath);

  return promise;
}
