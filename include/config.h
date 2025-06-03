#ifndef CONFIG_H
#define CONFIG_H

#define DEFAULT_PORT 8080
#define BUFFER_SIZE 1024
#define MAX_RESPONSE_SIZE 4096
#define MAX_PATH_LENGTH 256
#define MAX_METHOD_LENGTH 16
#define MAX_CONNECTIONS 10

// Database SettingsAdd commentMore actions
#define DB_NAME "httpserver.db"
#define ENABLE_WAL_MODE 1

#define STATIC_FILES_DIR "./public"
#define MAX_FILE_SIZE (10 * 1024 * 1024) // 10MB max file size

#endif // CONFIG_H