#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/database.h"

int db_init(Database *db, const char *path)
{
    if (!db || !path)
        return -1;

    // Store database path
    db->db_path = malloc(strlen(path) + 1);
    if (!db->db_path)
        return -1;

    strcpy(db->db_path, path);

    // Open database connection
    int rc = sqlite3_open(path, &db->db);
    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db->db));
        free(db->db_path);
        db->db_path = NULL;
        return -1;
    }

    printf("Database opened successfully: %s\n", path);
    return 0;
}

int db_create_tables(Database *db)
{
    if (!db || !db->db)
        return -1;

    const char *create_users_table =
        "CREATE TABLE IF NOT EXISTS users ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "name TEXT UNIQUE NOT NULL,"
        "email TEXT UNIQUE NOT NULL,"
        "password TEXT NOT NULL,"
        "created_at DATETIME DEFAULT CURRENT_TIMESTAMP"
        ");";

    char *err_msg = 0;
    int rc = sqlite3_exec(db->db, create_users_table, 0, 0, &err_msg);

    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
        return -1;
    }

    printf("Users table created successfully\n");
    return 0;
}

int db_create_user(Database *db, const char *name, const char *email, const char *password)
{
    if (!db || !db->db || !name || !email || !password)
        return -1;

    const char *sql = "INSERT INTO users (name, email, password) VALUES (?, ?, ?);";
    sqlite3_stmt *stmt;

    int rc = sqlite3_prepare_v2(db->db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db->db));
        return -1;
    }

    // Bind parameters
    sqlite3_bind_text(stmt, 1, name, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, email, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, password, -1, SQLITE_STATIC);

    // Execute the statement
    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE)
    {
        fprintf(stderr, "Failed to insert user: %s\n", sqlite3_errmsg(db->db));
        sqlite3_finalize(stmt);
        return -1;
    }

    // Get the inserted user ID
    int user_id = (int)sqlite3_last_insert_rowid(db->db);

    sqlite3_finalize(stmt);
    printf("User created with ID: %d\n", user_id);
    return user_id;
}

int db_get_users(Database *db, char *json_output, int max_len)
{
    if (!db || !db->db || !json_output || max_len <= 0)
    {
        return -1;
    }

    const char *sql = "SELECT id, name, email, created_at FROM users ORDER BY id;";
    sqlite3_stmt *stmt;

    int rc = sqlite3_prepare_v2(db->db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db->db));
        return -1;
    }

    // Start building JSON
    int written = snprintf(json_output, max_len, "{\"users\":[");
    if (written >= max_len)
    {
        sqlite3_finalize(stmt);
        return -1;
    }

    int first = 1;
    int user_count = 0;

    // Process each row
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW)
    {
        int id = sqlite3_column_int(stmt, 0);
        const char *name = (const char *)sqlite3_column_text(stmt, 1);
        const char *email = (const char *)sqlite3_column_text(stmt, 2);
        const char *created_at = (const char *)sqlite3_column_text(stmt, 3);

        // Add comma for subsequent entries
        if (!first)
        {
            written += snprintf(json_output + written, max_len - written, ",");
            if (written >= max_len)
                break;
        }

        // Add user object
        int user_written = snprintf(json_output + written, max_len - written,
                                    "{\"id\":%d,\"name\":\"%s\",\"email\":\"%s\",\"created_at\":\"%s\"}",
                                    id, name ? name : "", email ? email : "", created_at ? created_at : "");

        written += user_written;
        if (written >= max_len)
            break;

        first = 0;
        user_count++;
    }

    if (rc != SQLITE_DONE && rc != SQLITE_ROW)
    {
        fprintf(stderr, "Error reading users: %s\n", sqlite3_errmsg(db->db));
        sqlite3_finalize(stmt);
        return -1;
    }

    // Close JSON
    written += snprintf(json_output + written, max_len - written, "],\"count\":%d}", user_count);

    sqlite3_finalize(stmt);

    if (written >= max_len)
    {
        return -1; // Buffer too small
    }

    return user_count;
}

int db_get_user_by_id(Database *db, int id, char *json_output, int max_len)
{
    if (!db || !db->db || !json_output || max_len <= 0 || id <= 0)
    {
        return -1;
    }

    const char *sql = "SELECT id, name, email, created_at FROM users WHERE id = ?;";
    sqlite3_stmt *stmt;

    int rc = sqlite3_prepare_v2(db->db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db->db));
        return -1;
    }

    sqlite3_bind_int(stmt, 1, id);

    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW)
    {
        // User found
        int user_id = sqlite3_column_int(stmt, 0);
        const char *name = (const char *)sqlite3_column_text(stmt, 1);
        const char *email = (const char *)sqlite3_column_text(stmt, 2);
        const char *created_at = (const char *)sqlite3_column_text(stmt, 3);

        int written = snprintf(json_output, max_len,
                               "{\"id\":%d,\"name\":\"%s\",\"email\":\"%s\",\"created_at\":\"%s\"}",
                               user_id, name ? name : "", email ? email : "", created_at ? created_at : "");

        sqlite3_finalize(stmt);

        if (written >= max_len)
        {
            return -1; // Buffer too small
        }
        return 1; // User found
    }
    else if (rc == SQLITE_DONE)
    {
        // No user found
        snprintf(json_output, max_len, "{\"error\":\"User not found\"}");
        sqlite3_finalize(stmt);
        return 0; // User not found
    }
    else
    {
        // Error
        fprintf(stderr, "Error querying user: %s\n", sqlite3_errmsg(db->db));
        sqlite3_finalize(stmt);
        return -1;
    }
}

int db_update_user(Database *db, int id, const char *name, const char *email)
{
    if (!db || !db->db || id <= 0 || !name || !email)
    {
        return -1;
    }

    const char *sql = "UPDATE users SET name = ?, email = ? WHERE id = ?;";
    sqlite3_stmt *stmt;

    int rc = sqlite3_prepare_v2(db->db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db->db));
        return -1;
    }

    // Bind parameters
    sqlite3_bind_text(stmt, 1, name, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, email, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 3, id);

    // Execute the statement
    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE)
    {
        fprintf(stderr, "Failed to update user: %s\n", sqlite3_errmsg(db->db));
        sqlite3_finalize(stmt);
        return -1;
    }

    // Check if any rows were affected
    int rows_affected = sqlite3_changes(db->db);
    sqlite3_finalize(stmt);

    if (rows_affected > 0)
    {
        printf("User %d updated successfully\n", id);
        return 1; // Success
    }
    else
    {
        printf("User %d not found for update\n", id);
        return 0; // User not found
    }
}

int db_delete_user(Database *db, int id)
{
    if (!db || !db->db || id <= 0)
    {
        return -1;
    }

    const char *sql = "DELETE FROM users WHERE id = ?;";
    sqlite3_stmt *stmt;

    int rc = sqlite3_prepare_v2(db->db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db->db));
        return -1;
    }

    sqlite3_bind_int(stmt, 1, id);

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE)
    {
        fprintf(stderr, "Failed to delete user: %s\n", sqlite3_errmsg(db->db));
        sqlite3_finalize(stmt);
        return -1;
    }

    int rows_affected = sqlite3_changes(db->db);
    sqlite3_finalize(stmt);

    if (rows_affected > 0)
    {
        printf("User %d deleted successfully\n", id);
        return 1; // Success
    }
    else
    {
        printf("User %d not found for deletion\n", id);
        return 0; // User not found
    }
}

void db_close(Database *db)
{
    if (!db)
    {
        return;
    }

    if (db->db)
    {
        sqlite3_close(db->db);
        db->db = NULL;
    }

    if (db->db_path)
    {
        free(db->db_path);
        db->db_path = NULL;
    }

    printf("Database connection closed\n");
}

// Utility function to escape JSON strings
// static void escape_json_string(const char *input, char *output, int max_len)
// {
//     int i = 0, j = 0;
//     while (input[i] && j < max_len - 2)
//     {
//         if (input[i] == '"' || input[i] == '\\')
//         {
//             output[j++] = '\\';
//         }
//         output[j++] = input[i++];
//     }
//     output[j] = '\0';
// }