#include "api/streams_api/queue.h"
#include <stdlib.h>
#include <string.h>

void stream_queue_init(StreamQueue *queue) {
  queue->head = NULL;
  queue->tail = NULL;
  queue->total_size = 0;
}

void stream_queue_enqueue(StreamQueue *queue, char *data, size_t length) {
  StreamChunk *chunk = malloc(sizeof(StreamChunk));
  chunk->data = data;
  chunk->length = length;
  chunk->next = NULL;

  if (!queue->head) {
    queue->head = queue->tail = chunk;
  } else {
    queue->tail->next = chunk;
    queue->tail = chunk;
  }

  queue->total_size += length;
}

StreamChunk *stream_queue_dequeue(StreamQueue *queue) {
  if (!queue->head) {
    return NULL;
  }

  StreamChunk *chunk = queue->head;
  queue->head = chunk->next;
  
  if (!queue->head) {
    queue->tail = NULL;
  }

  queue->total_size -= chunk->length;
  return chunk;
}

void stream_queue_free(StreamQueue *queue) {
  StreamChunk *chunk = queue->head;
  while (chunk) {
    StreamChunk *next = chunk->next;
    free(chunk->data);
    free(chunk);
    chunk = next;
  }
  
  queue->head = NULL;
  queue->tail = NULL;
  queue->total_size = 0;
}

int stream_queue_is_empty(StreamQueue *queue) {
  return queue->head == NULL;
}