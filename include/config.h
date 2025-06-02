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

#endif // CONFIG_H