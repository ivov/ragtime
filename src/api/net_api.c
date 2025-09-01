#include "api/net_api.h"
#include "constants.h"
#include "core/jsc_interop.h"

#include <JavaScriptCore/JavaScript.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <uv.h>

static JSClassRef server_class = NULL;
static JSClassRef client_class = NULL;

typedef struct {
  uv_tcp_t socket;
  JSContextRef ctx;
  JSObjectRef callback;
} TcpServerState;

typedef struct {
  uv_tcp_t *socket;
} TcpClientState;

static void on_client_write(uv_write_t *write_req, int status) {
  free(write_req->data);
  free(write_req);
}

static void tcp_server_finalize(JSObjectRef object) {
  TcpServerState *state = (TcpServerState *)JSObjectGetPrivate(object);
  if (state) {
    JSValueUnprotect(state->ctx, state->callback);
    free(state);
  }
}

static void on_new_tcp_connection(uv_stream_t *server_socket, int uv_status) {
  if (uv_status < 0) {
    return;
  }

  TcpServerState *server_state = (TcpServerState *)server_socket->data;
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

  if (client_class == NULL) {
    JSClassDefinition class_def = kJSClassDefinitionEmpty;
    class_def.finalize = tcp_server_finalize;
    client_class = JSClassCreate(&class_def);
  }

  JSObjectRef client_obj =
      JSObjectMake(server_state->ctx, client_class, client_state);

  JSStringRef write_name = JSStringCreateWithUTF8CString("write");
  JSObjectRef write_fn = JSObjectMakeFunctionWithCallback(
      server_state->ctx, write_name, client_write);
  JSObjectSetProperty(server_state->ctx, client_obj, write_name, write_fn,
                      kJSPropertyAttributeNone, NULL);
  JSStringRelease(write_name);

  JSStringRef id_key = JSStringCreateWithUTF8CString("id");
  JSValueRef id_value =
      JSValueMakeNumber(server_state->ctx, (double)(uintptr_t)client_socket);
  JSObjectSetProperty(server_state->ctx, client_obj, id_key, id_value,
                      kJSPropertyAttributeNone, NULL);
  JSStringRelease(id_key);

  JSValueRef args[] = {client_obj};
  JSObjectCallAsFunction(server_state->ctx, server_state->callback, NULL, 1,
                         args, NULL);
}

JSValueRef net_create_server(JSContextRef ctx, JSObjectRef js_fn,
                             JSObjectRef this_obj, size_t argc,
                             const JSValueRef args[], JSValueRef *js_err_str) {
  if (argc != 1 || !JSObjectIsFunction(ctx, (JSObjectRef)args[0])) {
    set_js_error(ctx, ERR_CALLBACK_REQUIRED, js_err_str);
    return JSValueMakeUndefined(ctx);
  }

  TcpServerState *server_state = malloc(sizeof(TcpServerState));
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
    class_def.finalize = tcp_server_finalize;
    server_class = JSClassCreate(&class_def);
  }

  JSObjectRef server_obj = JSObjectMake(ctx, server_class, server_state);
  JSStringRef listen_name = JSStringCreateWithUTF8CString("listen");
  JSObjectRef listen_fn =
      JSObjectMakeFunctionWithCallback(ctx, listen_name, net_server_listen);
  JSObjectSetProperty(ctx, server_obj, listen_name, listen_fn,
                      kJSPropertyAttributeNone, NULL);
  JSStringRelease(listen_name);

  return server_obj;
}

JSValueRef net_server_listen(JSContextRef ctx, JSObjectRef js_fn,
                             JSObjectRef this_obj, size_t argc,
                             const JSValueRef args[], JSValueRef *js_err_str) {
  if (argc != 2 || !JSValueIsNumber(ctx, args[0]) ||
      !JSObjectIsFunction(ctx, (JSObjectRef)args[1])) {
    set_js_error(ctx, "Port and callback required", js_err_str);
    return JSValueMakeUndefined(ctx);
  }

  TcpServerState *server_state = (TcpServerState *)JSObjectGetPrivate(this_obj);
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

  int uv_listen_result = uv_listen((uv_stream_t *)&server_state->socket,
                                   TCP_LISTEN_BACKLOG, on_new_tcp_connection);
  if (uv_listen_result < 0) {
    fprintf(stderr, "Server listen error: %s\n", uv_strerror(uv_listen_result));
  }

  return JSValueMakeUndefined(ctx);
}

JSValueRef client_write(JSContextRef ctx, JSObjectRef js_fn,
                        JSObjectRef this_obj, size_t argc,
                        const JSValueRef args[], JSValueRef *js_err_str) {
  if (argc != 1 || !JSValueIsString(ctx, args[0])) {
    set_js_error(ctx, "String arg required", js_err_str);
    return JSValueMakeUndefined(ctx);
  }

  TcpClientState *client_state = (TcpClientState *)JSObjectGetPrivate(this_obj);
  if (!client_state || !client_state->socket) {
    set_js_error(ctx, ERR_INVALID_CLIENT_STATE, js_err_str);
    return JSValueMakeUndefined(ctx);
  }

  JSStringRef js_data_to_write = JSValueToStringCopy(ctx, args[0], js_err_str);
  if (!js_data_to_write || *js_err_str) {
    return JSValueMakeUndefined(ctx);
  }

  size_t js_data_to_write_len =
      JSStringGetMaximumUTF8CStringSize(js_data_to_write);
  char *c_data_to_write = (char *)malloc(js_data_to_write_len);
  if (!c_data_to_write) {
    JSStringRelease(js_data_to_write);
    set_js_error(ctx, ERR_MEMORY_ALLOCATION, js_err_str);
    return JSValueMakeUndefined(ctx);
  }

  JSStringGetUTF8CString(js_data_to_write, c_data_to_write,
                         js_data_to_write_len);
  JSStringRelease(js_data_to_write);

  uv_write_t *write_req = (uv_write_t *)malloc(sizeof(uv_write_t));
  if (!write_req) {
    free(c_data_to_write);
    set_js_error(ctx, ERR_MEMORY_ALLOCATION, js_err_str);
    return JSValueMakeUndefined(ctx);
  }

  uv_buf_t buffer = uv_buf_init(c_data_to_write, strlen(c_data_to_write));
  write_req->data = c_data_to_write; // for cleanup tracking

  uv_write(write_req, (uv_stream_t *)client_state->socket, &buffer, 1,
           on_client_write);

  return JSValueMakeUndefined(ctx);
}
