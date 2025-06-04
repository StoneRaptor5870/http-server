#ifndef CONFIG_H
#define CONFIG_H

#define DEFAULT_PORT 8080
#define BUFFER_SIZE 1024
#define MAX_RESPONSE_SIZE 4096
#define MAX_PATH_LENGTH 256
#define MAX_METHOD_LENGTH 16
#define MAX_CONNECTIONS 10

#define MAX_REQUEST_SIZE 8192
#define MAX_HEADER_LENGTH 1024
#define MAX_PARAM_KEY_LENGTH 64
#define MAX_PARAM_VALUE_LENGTH 256

#define MAX_URL_PARAMS 5
#define MAX_QUERY_PARAMS 10

// Database Settings
#define DB_NAME "httpserver.db"
#define ENABLE_WAL_MODE 1

#define STATIC_FILES_DIR "./public"
#define MAX_FILE_SIZE (10 * 1024 * 1024) // 10MB max file size

// Threadpool
#define DEFAULT_THREAD_COUNT 5
#define MAX_QUEUE_SIZE 100

#endif // CONFIG_H