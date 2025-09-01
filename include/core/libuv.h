#ifndef CORE_LIBUV_H
#define CORE_LIBUV_H

#include <uv.h>

extern uv_loop_t *loop;
void init_event_loop();
void run_event_loop();

#endif
