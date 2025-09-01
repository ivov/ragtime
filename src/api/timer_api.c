#include "api/timer_api.h"

#include "constants.h"
#include "core/jsc_interop.h"
#include "core/libuv.h"

#include <JavaScriptCore/JavaScript.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <uv.h>

extern uv_loop_t *loop;

typedef struct {
  uv_timer_t uv_handle;
  JSContextRef ctx;
  JSObjectRef callback;
  uint32_t id;
} TimerState;

static TimerState *timer_states[MAX_TIMER_STATES];
uint32_t next_timer_id = 0;

static void on_timer_close(uv_handle_t *uv_handle) {
  TimerState *state = (TimerState *)uv_handle->data;
  if (!state) {
    return;
  }
  JSValueUnprotect(state->ctx, state->callback);
  free(state);
}

static void on_timer(uv_timer_t *uv_handle) {
  TimerState *state = (TimerState *)uv_handle->data;
  if (!state) {
    return;
  }

  JSValueRef args[] = {JSValueMakeNumber(state->ctx, 0)};
  JSObjectCallAsFunction(state->ctx, state->callback, NULL, 1, args, NULL);

  if (uv_timer_get_repeat(uv_handle) == 0 /* one-shot timer */) {
    timer_states[state->id] = NULL;
    uv_close((uv_handle_t *)uv_handle, on_timer_close);
  }
}

static bool to_timer_id(JSContextRef ctx, JSValueRef js_timer_id,
                        uint32_t *timer_id_out, JSValueRef *js_err_str) {
  *timer_id_out = (uint32_t)JSValueToNumber(ctx, js_timer_id, js_err_str);
  if (*js_err_str) {
    return false;
  }

  return *timer_id_out < next_timer_id && timer_states[*timer_id_out] != NULL;
}

static bool to_duration(JSContextRef ctx, JSValueRef js_duration_ms,
                        uint64_t *duration_ms_out, JSValueRef *js_err_str) {
  double duration_ms = JSValueToNumber(ctx, js_duration_ms, js_err_str);
  if (*js_err_str) {
    return false;
  }

  if (duration_ms < 0) {
    JSStringRef err_msg =
        JSStringCreateWithUTF8CString("Delay must be non-negative");
    *js_err_str = JSValueMakeString(ctx, err_msg);
    JSStringRelease(err_msg);
    return false;
  }

  *duration_ms_out = (uint64_t)duration_ms;
  return true;
}

JSValueRef js_set_timeout(JSContextRef ctx, JSObjectRef fn,
                          JSObjectRef this_obj, size_t argc,
                          const JSValueRef args[], JSValueRef *js_err_str) {
  if (!is_valid_argc(ctx, argc, 2, "setTimeout", js_err_str)) {
    return JSValueMakeUndefined(ctx);
  }

  JSObjectRef callback;
  if (!to_callback(ctx, args[0], &callback, js_err_str)) {
    return JSValueMakeUndefined(ctx);
  }

  uint64_t delay_ms;
  if (!to_duration(ctx, args[1], &delay_ms, js_err_str)) {
    return JSValueMakeUndefined(ctx);
  }

  if (next_timer_id >= MAX_TIMER_STATES) {
    JSStringRef err_msg =
        JSStringCreateWithUTF8CString(ERR_TOO_MANY_TIMERS);
    *js_err_str = JSValueMakeString(ctx, err_msg);
    JSStringRelease(err_msg);
    return JSValueMakeUndefined(ctx);
  }

  TimerState *state = malloc(sizeof(TimerState));
  if (!state) {
    JSStringRef msg = JSStringCreateWithUTF8CString(ERR_MEMORY_ALLOCATION);
    *js_err_str = JSValueMakeString(ctx, msg);
    JSStringRelease(msg);
    return JSValueMakeUndefined(ctx);
  }

  state->ctx = ctx;
  state->callback = callback;
  state->id = next_timer_id++;
  timer_states[state->id] = state;

  uv_timer_init(loop, &state->uv_handle);
  state->uv_handle.data = state; // back pointer for later access
  uv_timer_start(&state->uv_handle, on_timer, delay_ms, 0 /* one-shot timer */);
  JSValueProtect(ctx, callback);

  return JSValueMakeNumber(ctx, state->id);
}

JSValueRef js_set_interval(JSContextRef ctx, JSObjectRef fn,
                           JSObjectRef this_obj, size_t argc,
                           const JSValueRef args[], JSValueRef *js_err_str) {
  if (!is_valid_argc(ctx, argc, 2, "setInterval", js_err_str)) {
    return JSValueMakeUndefined(ctx);
  }

  JSObjectRef callback;
  if (!to_callback(ctx, args[0], &callback, js_err_str)) {
    return JSValueMakeUndefined(ctx);
  }

  uint64_t interval_ms;
  if (!to_duration(ctx, args[1], &interval_ms, js_err_str)) {
    return JSValueMakeUndefined(ctx);
  }

  if (next_timer_id >= MAX_TIMER_STATES) {
    JSStringRef err_msg =
        JSStringCreateWithUTF8CString(ERR_TOO_MANY_TIMERS);
    *js_err_str = JSValueMakeString(ctx, err_msg);
    JSStringRelease(err_msg);
    return JSValueMakeUndefined(ctx);
  }

  TimerState *state = malloc(sizeof(TimerState));
  if (!state) {
    JSStringRef msg = JSStringCreateWithUTF8CString(ERR_MEMORY_ALLOCATION);
    *js_err_str = JSValueMakeString(ctx, msg);
    JSStringRelease(msg);
    return JSValueMakeUndefined(ctx);
  }

  state->ctx = ctx;
  state->callback = callback;
  state->id = next_timer_id++;
  timer_states[state->id] = state;

  uv_timer_init(loop, &state->uv_handle);
  state->uv_handle.data = state; // back pointer for later access
  uv_timer_start(&state->uv_handle, on_timer, interval_ms, interval_ms);
  JSValueProtect(ctx, callback);

  return JSValueMakeNumber(ctx, state->id);
}

JSValueRef js_clear_timeout(JSContextRef ctx, JSObjectRef js_fn,
                            JSObjectRef this_obj, size_t argc,
                            const JSValueRef args[], JSValueRef *js_err_str) {
  if (!is_valid_argc(ctx, argc, 1, "clearTimeout", js_err_str)) {
    return JSValueMakeUndefined(ctx);
  }

  uint32_t timer_id;
  if (!to_timer_id(ctx, args[0], &timer_id, js_err_str)) {
    return JSValueMakeUndefined(ctx);
  }

  TimerState *state = timer_states[timer_id];
  uv_timer_stop(&state->uv_handle);
  uv_close((uv_handle_t *)&state->uv_handle, on_timer_close);
  timer_states[timer_id] = NULL;

  return JSValueMakeUndefined(ctx);
}

JSValueRef js_clear_interval(JSContextRef ctx, JSObjectRef js_fn,
                             JSObjectRef this_obj, size_t argc,
                             const JSValueRef args[], JSValueRef *js_err_str) {
  if (!is_valid_argc(ctx, argc, 1, "clearInterval", js_err_str)) {
    return JSValueMakeUndefined(ctx);
  }

  uint32_t timer_id;
  if (!to_timer_id(ctx, args[0], &timer_id, js_err_str)) {
    return JSValueMakeUndefined(ctx);
  }

  TimerState *state = timer_states[timer_id];
  uv_timer_stop(&state->uv_handle);
  uv_close((uv_handle_t *)&state->uv_handle, on_timer_close);
  timer_states[timer_id] = NULL;

  return JSValueMakeUndefined(ctx);
}
