#include "api/http_api_common.h"

#include <stdlib.h>
#include <uv.h>

void http_write_complete(uv_write_t *req, int status) {
  free(req->data);
  free(req);
}

void http_alloc_buffer(uv_handle_t *handle, size_t suggested_size,
                       uv_buf_t *buf) {
  buf->base = (char *)malloc(suggested_size);
  if (buf->base == NULL) {
    buf->len = 0;
  } else {
    buf->len = suggested_size;
  }
}
