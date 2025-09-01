#ifndef API_HTTP_API_COMMON_H
#define API_HTTP_API_COMMON_H

#include <JavaScriptCore/JavaScript.h>
#include <uv.h>

void http_write_complete(uv_write_t *req, int status);
void http_alloc_buffer(uv_handle_t *handle, size_t suggested_size,
                       uv_buf_t *buf);

#endif
