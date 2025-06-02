#ifndef DATABASE_H
#define DATABASE_H

#include <sqlite3.h>
#include <time.h>
#include "config.h"

typedef struct
{
    sqlite3 *db;
    char *db_path;
} Database;

extern Database app_db;

int db_init(Database *db, const char *path);
int db_create_tables(Database *db);

// User operations
int db_create_user(Database *db, const char *name, const char *email, const char *password);
int db_get_users(Database *db, char *json_output, int max_len);
int db_get_user_by_id(Database *db, int id, char *json_output, int max_len);
int db_update_user(Database *db, int id, const char *username, const char *email);
int db_delete_user(Database *db, int id);

void db_close(Database *db);

#endif // DATABASE_H