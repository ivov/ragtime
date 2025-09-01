#include "core/libuv.h"

uv_loop_t *loop = NULL;

void init_event_loop(void) { loop = uv_default_loop(); }

void run_event_loop(void) { uv_run(loop, UV_RUN_DEFAULT); }
