#include "api/http_api.h"

#include "api/http_api_common.h"
#include "constants.h"
#include "core/jsc_interop.h"

#include <JavaScriptCore/JSObjectRef.h>
#include <JavaScriptCore/JavaScript.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <uv.h>

static JSClassRef server_class = NULL;

typedef struct {
  uv_tcp_t socket;
  JSContextRef ctx;
  JSObjectRef callback;
} HttpServerState;

typedef struct {
  uv_tcp_t *socket;
  HttpServerState *server_state;
  JSObjectRef req;
  JSObjectRef res;
} TcpClientState;

static void http_server_finalize(JSObjectRef object) {
  HttpServerState *server = (HttpServerState *)JSObjectGetPrivate(object);
  if (server) {
    JSValueUnprotect(server->ctx, server->callback);
    uv_close((uv_handle_t *)&server->socket, NULL);
    free(server);
  }
}

static JSValueRef res_end(JSContextRef ctx, JSObjectRef js_fn,
                          JSObjectRef this_obj, size_t argc,
                          const JSValueRef args[], JSValueRef *js_err_str) {
  TcpClientState *client_state = JSObjectGetPrivate(this_obj);
  if (!client_state) {
    set_js_error(ctx, ERR_INVALID_CLIENT_STATE, js_err_str);
    return JSValueMakeUndefined(ctx);
  }

  const char *body = "Hello, world!";
  char response[HTTP_RESPONSE_BUFFER_SIZE];
  snprintf(response, sizeof(response),
           "HTTP/1.1 200 OK\r\n"
           "Content-Type: text/plain\r\n"
           "Content-Length: %zu\r\n"
           "\r\n%s",
           strlen(body), body);

  char *response_copy =
      strdup(response); // write to heap until freed by http_write_complete
  if (!response_copy) {
    set_js_error(ctx, ERR_MEMORY_ALLOCATION, js_err_str);
    return JSValueMakeUndefined(ctx);
  }

  uv_write_t *write_req = malloc(sizeof(uv_write_t));
  if (!write_req) {
    free(response_copy);
    set_js_error(ctx, ERR_MEMORY_ALLOCATION, js_err_str);
    return JSValueMakeUndefined(ctx);
  }

  uv_buf_t buffer = uv_buf_init(response_copy, strlen(response));
  write_req->data = buffer.base;

  int write_result = uv_write(write_req, (uv_stream_t *)client_state->socket,
                              &buffer, 1, http_write_complete);
  if (write_result < 0) {
    fprintf(stderr, "Response write error: %s\n", uv_strerror(write_result));
    free(response_copy);
    free(write_req);
  }

  return JSValueMakeUndefined(ctx);
}

static void on_client_read(uv_stream_t *client_socket, ssize_t bytes_read,
                           const uv_buf_t *buffer) {
  TcpClientState *client_state = client_socket->data;

  if (bytes_read <= 0) {
    free(buffer->base);
    uv_close((uv_handle_t *)client_socket, NULL);
    free(client_state);
    return;
  }

  if ((size_t)bytes_read >= buffer->len) {
    free(buffer->base);
    return;
  }

  buffer->base[bytes_read] = '\0';

  char c_method[HTTP_METHOD_BUFFER_SIZE], c_url[HTTP_URL_BUFFER_SIZE];
  char *first_line = strtok(buffer->base, "\r\n");

  if (!first_line || sscanf(first_line, "%15s %255s", c_method, c_url) != 2) {
    fprintf(stderr, "Failed to parse HTTP request\n");
    free(buffer->base);
    return;
  }

  JSStringRef js_method = JSStringCreateWithUTF8CString(c_method);
  JSStringRef method_name = JSStringCreateWithUTF8CString("method");
  JSObjectSetProperty(
      client_state->server_state->ctx, client_state->req, method_name,
      JSValueMakeString(client_state->server_state->ctx, js_method),
      kJSPropertyAttributeNone, NULL);
  JSStringRelease(method_name);
  JSStringRelease(js_method);

  JSStringRef js_url = JSStringCreateWithUTF8CString(c_url);
  JSStringRef url_name = JSStringCreateWithUTF8CString("url");
  JSObjectSetProperty(
      client_state->server_state->ctx, client_state->req, url_name,
      JSValueMakeString(client_state->server_state->ctx, js_url),
      kJSPropertyAttributeNone, NULL);
  JSStringRelease(url_name);
  JSStringRelease(js_url);

  printf("Received request %s %s\n", c_method, c_url);

  JSValueRef args[] = {client_state->req, client_state->res};
  JSObjectCallAsFunction(client_state->server_state->ctx,
                         client_state->server_state->callback, NULL, 2, args,
                         NULL);

  free(buffer->base);
}

void on_new_http_connection(uv_stream_t *server_socket, int uv_status) {
  if (uv_status < 0) {
    fprintf(stderr, "New connection error: %s\n", uv_strerror(uv_status));
    return;
  }

  HttpServerState *server_state = server_socket->data;
  if (!server_state) {
    return;
  }

  uv_tcp_t *client_socket = malloc(sizeof(uv_tcp_t));
  if (!client_socket) {
    fprintf(stderr, "Failed to allocate memory for client socket\n");
    return;
  }

  uv_tcp_init(uv_default_loop(), client_socket);

  if (uv_accept(server_socket, (uv_stream_t *)client_socket) != 0) {
    uv_close((uv_handle_t *)client_socket, NULL);
    free(client_socket);
    return;
  }

  TcpClientState *client_state = malloc(sizeof(TcpClientState));
  if (!client_state) {
    uv_close((uv_handle_t *)client_socket, NULL);
    free(client_socket);
    return;
  }

  client_state->socket = client_socket;
  client_state->server_state = server_state;

  client_state->req = JSObjectMake(server_state->ctx, NULL, NULL);
  client_state->res = JSObjectMake(server_state->ctx, NULL, NULL);

  JSStringRef end_name = JSStringCreateWithUTF8CString("end");
  JSObjectRef end_fn =
      JSObjectMakeFunctionWithCallback(server_state->ctx, end_name, res_end);
  JSObjectSetProperty(server_state->ctx, client_state->res, end_name, end_fn,
                      kJSPropertyAttributeNone, NULL);
  JSStringRelease(end_name);

  JSObjectSetPrivate(client_state->res, client_state);

  client_socket->data = client_state;

  int read_result = uv_read_start((uv_stream_t *)client_socket,
                                  http_alloc_buffer, on_client_read);
  if (read_result < 0) {
    fprintf(stderr, "Failed to start reading from client: %s\n",
            uv_strerror(read_result));
    uv_close((uv_handle_t *)client_socket, NULL);
    free(client_state);
    free(client_socket);
  }
}

JSValueRef http_create_server(JSContextRef ctx, JSObjectRef js_fn,
                              JSObjectRef this_obj, size_t argc,
                              const JSValueRef args[], JSValueRef *js_err_str) {
  if (argc != 1 || !JSObjectIsFunction(ctx, (JSObjectRef)args[0])) {
    set_js_error(ctx, ERR_CALLBACK_REQUIRED, js_err_str);
    return JSValueMakeUndefined(ctx);
  }

  HttpServerState *server_state = malloc(sizeof(HttpServerState));
  if (!server_state) {
    set_js_error(ctx, ERR_MEMORY_ALLOCATION, js_err_str);
    return JSValueMakeUndefined(ctx);
  }

  server_state->ctx = ctx;
  server_state->callback = (JSObjectRef)args[0];
  JSValueProtect(ctx, server_state->callback);

  uv_tcp_init(uv_default_loop(), &server_state->socket);
  server_state->socket.data = server_state; // back pointer for later access

  if (server_class == NULL) {
    JSClassDefinition class_def = kJSClassDefinitionEmpty;
    class_def.finalize = http_server_finalize;
    server_class = JSClassCreate(&class_def);
  }

  JSObjectRef server_obj = JSObjectMake(ctx, server_class, server_state);
  JSStringRef listen_name = JSStringCreateWithUTF8CString("listen");
  JSObjectRef listen_fn =
      JSObjectMakeFunctionWithCallback(ctx, listen_name, http_server_listen);
  JSObjectSetProperty(ctx, server_obj, listen_name, listen_fn,
                      kJSPropertyAttributeNone, NULL);
  JSStringRelease(listen_name);

  return server_obj;
}

JSValueRef http_server_listen(JSContextRef ctx, JSObjectRef js_fn,
                              JSObjectRef this_obj, size_t argc,
                              const JSValueRef args[], JSValueRef *js_err_str) {
  if (argc != 1 || !JSValueIsNumber(ctx, args[0])) {
    set_js_error(ctx, "Port required", js_err_str);
    return JSValueMakeUndefined(ctx);
  }

  HttpServerState *server_state = JSObjectGetPrivate(this_obj);
  if (!server_state) {
    set_js_error(ctx, ERR_INVALID_SERVER_STATE, js_err_str);
    return JSValueMakeUndefined(ctx);
  }

  int port = JSValueToNumber(ctx, args[0], js_err_str);
  if (*js_err_str) {
    return JSValueMakeUndefined(ctx);
  }

  struct sockaddr_in addr;
  uv_ip4_addr("0.0.0.0", port, &addr);

  int bind_result =
      uv_tcp_bind(&server_state->socket, (const struct sockaddr *)&addr, 0);
  if (bind_result < 0) {
    fprintf(stderr, "Server socket bind error: %s\n", uv_strerror(bind_result));
    return JSValueMakeUndefined(ctx);
  }

  int listen_result = uv_listen((uv_stream_t *)&server_state->socket,
                                TCP_LISTEN_BACKLOG, on_new_http_connection);
  if (listen_result < 0) {
    fprintf(stderr, "Server listen error: %s\n", uv_strerror(listen_result));
    return JSValueMakeUndefined(ctx);
  }

  printf("HTTP server listening on port %d\n", port);
  return JSValueMakeUndefined(ctx);
}
