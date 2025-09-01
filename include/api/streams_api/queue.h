#ifndef STREAMS_API_QUEUE_H
#define STREAMS_API_QUEUE_H

#include <stddef.h>

typedef struct StreamChunk {
  char *data;
  size_t length;
  struct StreamChunk *next;
} StreamChunk;

typedef struct StreamQueue {
  StreamChunk *head;
  StreamChunk *tail;
  size_t total_size;
} StreamQueue;

void stream_queue_init(StreamQueue *queue);
void stream_queue_enqueue(StreamQueue *queue, char *data, size_t length);
StreamChunk *stream_queue_dequeue(StreamQueue *queue);
void stream_queue_free(StreamQueue *queue);
int stream_queue_is_empty(StreamQueue *queue);

#endif