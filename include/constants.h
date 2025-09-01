#ifndef CONSTANTS_H
#define CONSTANTS_H

// Runtime
#define RUNTIME_VERSION "0.1.0"
#define RUNTIME_NAME "ragtime"

// Limits
#define MAX_FILE_SIZE 1024    // bytes
#define TCP_LISTEN_BACKLOG 10 // pending connections
#define MAX_TIMER_STATES 1024 // active timers
#define MODULE_CACHE_SIZE 64  // hashtable buckets

// Stream limits
#define STREAM_CHUNK_SIZE 4096 // 4 KiB
#define STREAM_HIGH_WATERMARK 16384 // 16 KiB
#define MAX_STREAM_QUEUE_SIZE 32

// Buffer sizes
#define HTTP_REQUEST_BUFFER_SIZE 1024
#define HTTP_RESPONSE_BUFFER_SIZE 512
#define HTTP_METHOD_BUFFER_SIZE 16
#define HTTP_URL_BUFFER_SIZE 256
#define ERROR_MSG_BUFFER_SIZE 512

// Network
#define HTTP_DEFAULT_PORT "80"
#define FILE_DEFAULT_PERMISSIONS 0644

// ANSI Colors
#define ANSI_RESET "\033[0m"
#define ANSI_RED "\033[31m"
#define ANSI_YELLOW "\033[33m"
#define ANSI_BLUE "\033[34m"
#define ANSI_GRAY "\033[90m"

// Error messages
#define ERR_MEMORY_ALLOCATION "Memory allocation failed"
#define ERR_INVALID_STREAM_STATE "Invalid stream state"
#define ERR_INVALID_SERVER_STATE "Invalid server state"
#define ERR_INVALID_CLIENT_STATE "Invalid client state"
#define ERR_CALLBACK_REQUIRED "Callback must be a function"
#define ERR_TOO_MANY_TIMERS "Too many active timers"

#endif
