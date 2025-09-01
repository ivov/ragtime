#include "api/http_api.h"

#include "api/http_api_common.h"
#include "constants.h"
#include "core/jsc_interop.h"

#include <JavaScriptCore/JavaScript.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <uv.h>

typedef struct {
  uv_tcp_t socket;
  JSContextRef ctx;
  JSObjectRef callback;
  char *host;
  char *path;
  char *response_data;
  size_t response_size;
  int status_code;
  char *response_headers;
  char *response_body;
  bool headers_parsed;
} HttpRequestState;

static void on_http_read(uv_stream_t *stream, ssize_t bytes_read,
                         const uv_buf_t *buf);
static void on_http_connect(uv_connect_t *req, int status);
static void on_dns_resolved(uv_getaddrinfo_t *resolver, int status,
                            struct addrinfo *res);
static void on_socket_close(uv_handle_t *handle);

static void invoke_callback_with_error(HttpRequestState *state,
                                       const char *err_msg) {
  JSStringRef err_str = JSStringCreateWithUTF8CString(err_msg);
  JSValueRef args[] = {JSValueMakeString(state->ctx, err_str),
                       JSValueMakeNull(state->ctx)};
  JSObjectCallAsFunction(state->ctx, state->callback, NULL, 2, args, NULL);
  JSStringRelease(err_str);
}

static void on_http_read(uv_stream_t *stream, ssize_t bytes_read,
                         const uv_buf_t *buffer) {
  HttpRequestState *state = stream->data;

  if (!validate_state(state, "on_http_read")) {
    free(buffer->base);
    return;
  }

  if (bytes_read < 0) {
    invoke_callback_with_error(state, uv_strerror(bytes_read));
    uv_close((uv_handle_t *)&state->socket, on_socket_close);
    JSValueUnprotect(state->ctx, state->callback);
    free(state->host);
    free(state->path);
    free(state->response_data);
    free(state->response_headers);
    free(state->response_body);
    free(state);
    free(buffer->base);
    return;
  }

  if (bytes_read > 0) {
    char *new_data =
        realloc(state->response_data, state->response_size + bytes_read + 1);
    if (!new_data) {
      invoke_callback_with_error(state, ERR_MEMORY_ALLOCATION);
      uv_close((uv_handle_t *)&state->socket, on_socket_close);
      JSValueUnprotect(state->ctx, state->callback);
      free(state->host);
      free(state->path);
      free(state->response_data);
      free(state->response_headers);
      free(state->response_body);
      free(state);
      free(buffer->base);
      return;
    }
    state->response_data = new_data;
    memcpy(state->response_data + state->response_size, buffer->base,
           bytes_read);
    state->response_size += bytes_read;
    state->response_data[state->response_size] = '\0';

    if (!state->headers_parsed) {
      char *header_end = strstr(state->response_data, "\r\n\r\n");
      if (header_end) {
        size_t header_len = header_end - state->response_data;

        state->response_headers = malloc(header_len + 1);
        if (!state->response_headers) {
          invoke_callback_with_error(state, ERR_MEMORY_ALLOCATION);
          return;
        }
        memcpy(state->response_headers, state->response_data, header_len);
        state->response_headers[header_len] = '\0';

        char *status_line = strtok(state->response_headers, "\r\n");
        if (status_line) {
          sscanf(status_line, "HTTP/1.1 %d", &state->status_code);
        }

        size_t body_start = header_len + 4; // Skip \r\n\r\n
        size_t body_len = state->response_size - body_start;
        if (body_len > 0) {
          state->response_body = malloc(body_len + 1);
          if (!state->response_body) {
            invoke_callback_with_error(state, ERR_MEMORY_ALLOCATION);
            return;
          }
          memcpy(state->response_body, state->response_data + body_start,
                 body_len);
          state->response_body[body_len] = '\0';
        } else {
          state->response_body = strdup("{}");
          if (!state->response_body) {
            invoke_callback_with_error(state, ERR_MEMORY_ALLOCATION);
            return;
          }
        }

        state->headers_parsed = true;
      }
    }
    free(buffer->base);
    return;
  }

  // bytes_read == 0 means EOF reached - send response to callback
  JSObjectRef response_obj = JSObjectMake(state->ctx, NULL, NULL);

  JSStringRef statusName = JSStringCreateWithUTF8CString("statusCode");
  JSObjectSetProperty(state->ctx, response_obj, statusName,
                      JSValueMakeNumber(state->ctx, state->status_code),
                      kJSPropertyAttributeNone, NULL);
  JSStringRelease(statusName);

  JSObjectRef headers_obj = JSObjectMake(state->ctx, NULL, NULL);
  if (state->response_headers) {
    char *header_line = strtok(state->response_headers, "\r\n");
    while (header_line) {
      char *colon = strchr(header_line, ':');
      if (colon) {
        *colon = '\0';
        char *value = colon + 1;
        while (*value == ' ') {
          value++;
        }

        JSStringRef headerName = JSStringCreateWithUTF8CString(header_line);
        JSStringRef headerValue = JSStringCreateWithUTF8CString(value);
        JSObjectSetProperty(state->ctx, headers_obj, headerName,
                            JSValueMakeString(state->ctx, headerValue),
                            kJSPropertyAttributeNone, NULL);
        JSStringRelease(headerName);
        JSStringRelease(headerValue);
      }
      header_line = strtok(NULL, "\r\n");
    }
  }
  JSStringRef headersName = JSStringCreateWithUTF8CString("headers");
  JSObjectSetProperty(state->ctx, response_obj, headersName,
                      JSValueToObject(state->ctx, headers_obj, NULL),
                      kJSPropertyAttributeNone, NULL);
  JSStringRelease(headersName);

  JSStringRef bodyName = JSStringCreateWithUTF8CString("body");
  JSStringRef bodyValue = JSStringCreateWithUTF8CString(
      state->response_body ? state->response_body : "{}");
  JSObjectSetProperty(state->ctx, response_obj, bodyName,
                      JSValueMakeString(state->ctx, bodyValue),
                      kJSPropertyAttributeNone, NULL);
  JSStringRelease(bodyName);
  JSStringRelease(bodyValue);

  JSValueRef args[] = {JSValueMakeNull(state->ctx),
                       JSValueToObject(state->ctx, response_obj, NULL)};
  JSObjectCallAsFunction(state->ctx, state->callback, NULL, 2, args, NULL);

  uv_close((uv_handle_t *)&state->socket, on_socket_close);
  JSValueUnprotect(state->ctx, state->callback);
  free(state->host);
  free(state->path);
  free(state->response_data);
  free(state->response_headers);
  free(state->response_body);
  free(state);
  free(buffer->base);
}

static void on_socket_close(uv_handle_t *handle) {
  // socket closed, no additional cleanup needed
}

static void on_http_connect(uv_connect_t *req, int uv_status) {
  HttpRequestState *state = (HttpRequestState *)req->data;
  if (!state) {
    free(req);
    return;
  }

  if (uv_status < 0) {
    invoke_callback_with_error(state, uv_strerror(uv_status));
    uv_close((uv_handle_t *)&state->socket, on_socket_close);
    JSValueUnprotect(state->ctx, state->callback);
    free(state->host);
    free(state->path);
    free(state);
    free(req);
    return;
  }

  char request[HTTP_REQUEST_BUFFER_SIZE];

  snprintf(request, sizeof(request),
           "GET %s HTTP/1.1\r\n"
           "Host: %s\r\n"
           "Connection: close\r\n\r\n",
           state->path, state->host);

  uv_buf_t write_buf = uv_buf_init(strdup(request), strlen(request));
  if (!write_buf.base) {
    invoke_callback_with_error(state, ERR_MEMORY_ALLOCATION);
    uv_close((uv_handle_t *)&state->socket, on_socket_close);
    JSValueUnprotect(state->ctx, state->callback);
    free(state->host);
    free(state->path);
    free(state);
    free(req);
    return;
  }
  uv_write_t *write_req = malloc(sizeof(uv_write_t));
  if (!write_req) {
    free(write_buf.base);
    invoke_callback_with_error(state, ERR_MEMORY_ALLOCATION);
    uv_close((uv_handle_t *)&state->socket, on_socket_close);
    JSValueUnprotect(state->ctx, state->callback);
    free(state->host);
    free(state->path);
    free(state);
    free(req);
    return;
  }
  write_req->data = write_buf.base;
  uv_write(write_req, req->handle, &write_buf, 1, http_write_complete);
  uv_read_start(req->handle, http_alloc_buffer, on_http_read);
  free(req);
}

static void on_dns_resolved(uv_getaddrinfo_t *resolver, int uv_status,
                            struct addrinfo *res) {
  HttpRequestState *state = resolver->data;

  if (uv_status < 0) {
    invoke_callback_with_error(state, uv_strerror(uv_status));
    JSValueUnprotect(state->ctx, state->callback);
    free(state->host);
    free(state->path);
    free(state);
    free(resolver);
    if (res) {
      uv_freeaddrinfo(res);
    }
    return;
  }

  uv_tcp_init(uv_default_loop(), &state->socket);
  state->socket.data = state;

  uv_connect_t *connect_req = malloc(sizeof(uv_connect_t));
  if (!connect_req) {
    invoke_callback_with_error(state, ERR_MEMORY_ALLOCATION);
    JSValueUnprotect(state->ctx, state->callback);
    free(state->host);
    free(state->path);
    free(state);
    uv_freeaddrinfo(res);
    free(resolver);
    return;
  }
  connect_req->data = state;
  uv_tcp_connect(connect_req, &state->socket,
                 (const struct sockaddr *)res->ai_addr, on_http_connect);
  uv_freeaddrinfo(res);
  free(resolver);
}

JSValueRef http_get(JSContextRef ctx, JSObjectRef js_fn, JSObjectRef this_obj,
                    size_t argc, const JSValueRef args[],
                    JSValueRef *js_err_str) {
  if (!is_valid_argc(ctx, argc, 2, "http.get", js_err_str)) {
    return JSValueMakeUndefined(ctx);
  }

  char *url = to_c_str(ctx, args[0], js_err_str);
  if (*js_err_str) {
    return JSValueMakeUndefined(ctx);
  }

  if (strncmp(url, "http://", 7) != 0) {
    free(url);
    JSStringRef err_msg =
        JSStringCreateWithUTF8CString("Only HTTP URLs are supported");
    *js_err_str = JSValueMakeString(ctx, err_msg);
    JSStringRelease(err_msg);
    return JSValueMakeUndefined(ctx);
  }

  JSObjectRef callback;
  if (!to_callback(ctx, args[1], &callback, js_err_str)) {
    free(url);
    return JSValueMakeUndefined(ctx);
  }

  char *host_start = url + 7;
  char *path_start = strchr(host_start, '/');
  char *host = path_start ? strndup(host_start, path_start - host_start)
                          : strdup(host_start);
  if (!host) {
    free(url);
    set_js_error(ctx, ERR_MEMORY_ALLOCATION, js_err_str);
    return JSValueMakeUndefined(ctx);
  }
  
  char *path = path_start ? strdup(path_start) : strdup("/");
  if (!path) {
    free(url);
    free(host);
    set_js_error(ctx, ERR_MEMORY_ALLOCATION, js_err_str);
    return JSValueMakeUndefined(ctx);
  }

  HttpRequestState *http = malloc(sizeof(HttpRequestState));
  if (!http) {
    free(url);
    free(host);
    free(path);
    set_js_error(ctx, ERR_MEMORY_ALLOCATION, js_err_str);
    return JSValueMakeUndefined(ctx);
  }

  http->ctx = ctx;
  http->callback = callback;
  JSValueProtect(ctx, callback);
  http->host = host;
  http->path = path;
  http->response_data = NULL;
  http->response_size = 0;
  http->status_code = 0;
  http->response_headers = NULL;
  http->response_body = NULL;
  http->headers_parsed = false;

  struct addrinfo hints = {.ai_family = AF_INET, .ai_socktype = SOCK_STREAM};
  uv_getaddrinfo_t *resolver = malloc(sizeof(uv_getaddrinfo_t));
  if (!resolver) {
    JSValueUnprotect(ctx, callback);
    free(url);
    free(host);
    free(path);
    free(http);
    set_js_error(ctx, ERR_MEMORY_ALLOCATION, js_err_str);
    return JSValueMakeUndefined(ctx);
  }
  resolver->data = http;
  uv_getaddrinfo(uv_default_loop(), resolver, on_dns_resolved, http->host, HTTP_DEFAULT_PORT,
                 &hints);

  free(url);
  return JSValueMakeUndefined(ctx);
}
