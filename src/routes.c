#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "../include/config.h"
#include "../include/routes.h"
#include "../include/database.h"
#include "../include/utils.h"
#include "../include/file.h"

static pthread_mutex_t db_mutex = PTHREAD_MUTEX_INITIALIZER;

void route_get_css(const HTTP_REQUEST *request, HTTP_RESPONSE *response)
{
    (void)request;

    if (strcmp(request->clean_path, "/css/style.css") == 0)
    {
        serve_static_file(request, response);
        return;
    }
}

void route_get_js(const HTTP_REQUEST *request, HTTP_RESPONSE *response)
{
    (void)request;

    if (strcmp(request->clean_path, "/js/app.js") == 0)
    {
        serve_static_file(request, response);
        return;
    }
}

void route_home(const HTTP_REQUEST *request, HTTP_RESPONSE *response)
{
    // For root path
    if (strcmp(request->clean_path, "/") == 0)
    {
        serve_static_file(request, response);
        return;
    }

    // If it's not an API route
    if (strncmp(request->clean_path, "/api/", 5) != 0)
    {
        serve_static_file(request, response);
        return;
    }

    // Fallback to original hardcoded HTML
    char body[2048];
    snprintf(body, sizeof(body),
             "<!DOCTYPE html>\n"
             "<html>\n"
             "<head><title>My C HTTP Server</title></head>\n"
             "<body>\n"
             "<h1>Hello from C HTTP Server!</h1>\n"
             "<p>This server is running on port %d</p>\n"
             "<p>Requested path: %s</p>\n"
             "<p><a href=\"/about\">About this server</a></p>\n"
             "</body>\n"
             "</html>\n",
             DEFAULT_PORT, request->path);

    http_response_set_status(response, HTTP_200_OK);
    http_response_set_content_type(response, "text/html");
    http_response_set_body(response, body);
}

void route_about(const HTTP_REQUEST *request, HTTP_RESPONSE *response)
{
    (void)request;

    if (strcmp(request->clean_path, "/about") == 0)
    {
        serve_static_file(request, response);
        return;
    }

    // If static file not found, serve hardcoded HTML instead
    if (response->status == HTTP_404_NOT_FOUND)
    {
        const char *body =
            "<!DOCTYPE html>\n"
            "<html>\n"
            "<head><title>About - My C HTTP Server</title></head>\n"
            "<body>\n"
            "<h1>About This Server</h1>\n"
            "<p>This is a HTTP server written in C.</p>\n"
            "<p>It demonstrates:</p>\n"
            "<ul>\n"
            "<li>Modular code organization</li>\n"
            "<li>TCP socket programming</li>\n"
            "<li>HTTP protocol handling (GET, POST, PUT, PATCH, DELETE)</li>\n"
            "<li>Request parsing and response generation</li>\n"
            "<li>JSON API endpoints</li>\n"
            "</ul>\n"
            "<p><a href=\"/\">Go back to home</a></p>\n"
            "</body>\n"
            "</html>\n";

        http_response_set_status(response, HTTP_200_OK);
        http_response_set_content_type(response, "text/html");
        http_response_set_body(response, body);
    }
}

// API Routes
void route_get_users(const HTTP_REQUEST *request, HTTP_RESPONSE *response)
{
    if (!request || !response)
    {
        return;
    }

    char json_buffer[4096];
    UserQueryParams params;

    // Initialize parameters
    init_user_query_params(&params);

    // Parse HTTP request into database parameters
    const char *limit_str = get_query_param(request, "limit");
    const char *offset_str = get_query_param(request, "offset");
    const char *search_str = get_query_param(request, "search");

    // Set pagination parameters
    if (limit_str)
    {
        int limit = atoi(limit_str);
        if (limit > 0 && limit <= 100) // Add reasonable upper limit
        {
            params.limit = limit;
        }
    }

    if (offset_str)
    {
        int offset = atoi(offset_str);
        if (offset >= 0)
        {
            params.offset = offset;
        }
    }

    // Set search parameter
    if (search_str && strlen(search_str) > 0)
    {
        strncpy(params.search, search_str, sizeof(params.search) - 1);
        params.search[sizeof(params.search) - 1] = '\0';
    }

    // Convert query parameters to filters
    for (int i = 0; i < request->query_param_count && params.filter_count < 10; i++)
    {
        const char *key = request->query_params[i].key;
        const char *value = request->query_params[i].value;

        // Skip reserved parameters
        if (strcmp(key, "limit") == 0 || strcmp(key, "offset") == 0 ||
            strcmp(key, "search") == 0 || strlen(value) == 0)
        {
            continue;
        }

        // Add to filters
        strncpy(params.filters[params.filter_count].key, key,
                sizeof(params.filters[params.filter_count].key) - 1);
        params.filters[params.filter_count].key[sizeof(params.filters[params.filter_count].key) - 1] = '\0';

        strncpy(params.filters[params.filter_count].value, value,
                sizeof(params.filters[params.filter_count].value) - 1);
        params.filters[params.filter_count].value[sizeof(params.filters[params.filter_count].value) - 1] = '\0';

        params.filter_count++;
    }

    printf("Getting users with limit=%d, offset=%d, filters=%d\n",
           params.limit, params.offset, params.filter_count);

    for (int i = 0; i < params.filter_count; i++)
    {
        printf("  Filter: %s = %s\n", params.filters[i].key, params.filters[i].value);
    }

    // Call database function
    pthread_mutex_lock(&db_mutex);
    int result = db_get_users(&app_db, json_buffer, sizeof(json_buffer), &params);
    pthread_mutex_unlock(&db_mutex);

    if (result >= 0)
    {
        http_response_set_status(response, HTTP_200_OK);
        http_response_set_content_type(response, "application/json");
        http_response_set_body(response, json_buffer);
    }
    else
    {
        const char *error_json = "{\"error\":\"Failed to retrieve users\"}";
        http_response_set_status(response, HTTP_500_INTERNAL_ERROR);
        http_response_set_content_type(response, "application/json");
        http_response_set_body(response, error_json);
    }
}

void route_get_user_by_id(const HTTP_REQUEST *request, HTTP_RESPONSE *response)
{
    if (!request || !response)
    {
        return;
    }

    char body[1024];
    char user_json[512];

    // Extract user ID from URL path "/api/users/123"
    // Get URL parameter safely
    const char *user_id_str = get_url_param(request, "id");
    if (!user_id_str || strlen(user_id_str) == 0)
    {
        snprintf(body, sizeof(body),
                 "{\n"
                 "  \"error\": \"Bad Request\",\n"
                 "  \"message\": \"User ID is missing\"\n"
                 "}");
        http_response_set_status(response, HTTP_400_BAD_REQUEST);
        http_response_set_content_type(response, "application/json");
        http_response_set_body(response, body);
        return;
    }

    char *endptr;
    int user_id = strtol(user_id_str, &endptr, 10);
    if (*endptr != '\0' || user_id <= 0)
    {
        snprintf(body, sizeof(body),
                 "{\n"
                 "  \"error\": \"Bad Request\",\n"
                 "  \"message\": \"Invalid user ID\"\n"
                 "}");
        http_response_set_status(response, HTTP_400_BAD_REQUEST);
        http_response_set_content_type(response, "application/json");
        http_response_set_body(response, body);
        return;
    }

    pthread_mutex_lock(&db_mutex);
    int result = db_get_user_by_id(&app_db, user_id, user_json, sizeof(user_json));
    pthread_mutex_unlock(&db_mutex);

    if (result > 0)
    {
        http_response_set_status(response, HTTP_200_OK);
        http_response_set_content_type(response, "application/json");
        http_response_set_body(response, user_json);
    }
    else if (result == 0)
    {
        snprintf(body, sizeof(body),
                 "{\n"
                 "  \"error\": \"Not Found\",\n"
                 "  \"message\": \"User with ID %d not found\"\n"
                 "}",
                 user_id);
        http_response_set_status(response, HTTP_404_NOT_FOUND);
        http_response_set_content_type(response, "application/json");
        http_response_set_body(response, body);
    }
    else
    {
        snprintf(body, sizeof(body),
                 "{\n"
                 "  \"error\": \"Internal Server Error\",\n"
                 "  \"message\": \"Database error occurred while retrieving user\"\n"
                 "}");
        http_response_set_status(response, HTTP_500_INTERNAL_ERROR);
        http_response_set_content_type(response, "application/json");
        http_response_set_body(response, body);
    }
}

void route_create_user(const HTTP_REQUEST *request, HTTP_RESPONSE *response)
{
    char body[1024];
    char name[256];
    char email[256];
    char password[256];

    // Check if request body exists
    if (request->body && request->content_length <= 0)
    {
        snprintf(body, sizeof(body),
                 "{\n"
                 "  \"error\": \"Bad Request\",\n"
                 "  \"message\": \"Request body is required for user creation\"\n"
                 "}");

        http_response_set_status(response, HTTP_400_BAD_REQUEST);
        http_response_set_content_type(response, "application/json");
        http_response_set_body(response, body);
        return;
    }

    printf("Creating user with data: %s\n", request->body);

    // Parse JSON data
    if (parse_user_json(request->body, name, sizeof(name), email, sizeof(email), password, sizeof(password)) < 0)
    {
        snprintf(body, sizeof(body),
                 "{\n"
                 "  \"error\": \"Bad Request\",\n"
                 "  \"message\": \"Invalid JSON format. Expected {\\\"name\\\":\\\"...\\\", \\\"email\\\":\\\"...\\\"}\"\n"
                 "}");

        http_response_set_status(response, HTTP_400_BAD_REQUEST);
        http_response_set_content_type(response, "application/json");
        http_response_set_body(response, body);
        return;
    }

    // Validate username
    if (!is_valid_name(name) || !is_valid_email(email) || !is_valid_password(password))
    {
        snprintf(body, sizeof(body),
                 "{\n"
                 "  \"error\": \"Bad Request\",\n"
                 "  \"message\": \"Name, email and password must be 3-50 characters long and contain only letters, numbers, and underscores\"\n"
                 "}");

        http_response_set_status(response, HTTP_400_BAD_REQUEST);
        http_response_set_content_type(response, "application/json");
        http_response_set_body(response, body);
        return;
    }

    // Create user in database
    pthread_mutex_lock(&db_mutex);
    int user_id = db_create_user(&app_db, name, email, password);
    pthread_mutex_unlock(&db_mutex);

    if (user_id > 0)
    {
        // Success - user created
        snprintf(body, sizeof(body),
                 "{\n"
                 "  \"message\": \"User created successfully\",\n"
                 "  \"id\": %d,\n"
                 "  \"name\": \"%s\",\n"
                 "  \"email\": \"%s\"\n"
                 "}",
                 user_id, name, email);

        http_response_set_status(response, HTTP_201_CREATED);
        printf("User created successfully with ID: %d\n", user_id);
    }
    else
    {
        // Database error or constraint violation (duplicate username/email)
        snprintf(body, sizeof(body),
                 "{\n"
                 "  \"error\": \"Conflict\",\n"
                 "  \"message\": \"Username or email already exists, or database error occurred\"\n"
                 "}");

        http_response_set_status(response, HTTP_409_CONFLICT);
        printf("Failed to create user - possibly duplicate username/email\n");
    }

    http_response_set_content_type(response, "application/json");
    http_response_set_body(response, body);
}

void route_update_user(const HTTP_REQUEST *request, HTTP_RESPONSE *response)
{
    char body[1024];
    char name[256];
    char email[256];
    char password[256];

    // Extract user ID from URL path "/api/users/123"
    const char *user_id_str = get_url_param(request, "id");
    int user_id = atoi(user_id_str);

    if (user_id <= 0)
    {
        snprintf(body, sizeof(body),
                 "{\n"
                 "  \"error\": \"Bad Request\",\n"
                 "  \"message\": \"Invalid user ID\"\n"
                 "}");

        http_response_set_status(response, HTTP_400_BAD_REQUEST);
        http_response_set_content_type(response, "application/json");
        http_response_set_body(response, body);
        return;
    }

    // Check if request body exists
    if (!request->body || request->content_length <= 0)
    {
        snprintf(body, sizeof(body),
                 "{\n"
                 "  \"error\": \"Bad Request\",\n"
                 "  \"message\": \"Request body is required for user update\"\n"
                 "}");

        http_response_set_status(response, HTTP_400_BAD_REQUEST);
        http_response_set_content_type(response, "application/json");
        http_response_set_body(response, body);
        return;
    }

    printf("Updating user %d with data: %s\n", user_id, request->body);

    // Parse JSON data
    if (parse_user_json(request->body, name, sizeof(name), email, sizeof(email), password, sizeof(password)) < 0)
    {
        snprintf(body, sizeof(body),
                 "{\n"
                 "  \"error\": \"Bad Request\",\n"
                 "  \"message\": \"Invalid JSON format. Expected {\\\"name\\\":\\\"...\\\", \\\"email\\\":\\\"...\\\", \\\"password\\\":\\\"...\\\"}\"\n"
                 "}");

        http_response_set_status(response, HTTP_400_BAD_REQUEST);
        http_response_set_content_type(response, "application/json");
        http_response_set_body(response, body);
        return;
    }

    // Validate input
    if (!is_valid_name(name) || !is_valid_email(email) || !is_valid_password(password))
    {
        snprintf(body, sizeof(body),
                 "{\n"
                 "  \"error\": \"Bad Request\",\n"
                 "  \"message\": \"Name, email and password must be 3-50 characters long and contain only letters, numbers, and underscores\"\n"
                 "}");

        http_response_set_status(response, HTTP_400_BAD_REQUEST);
        http_response_set_content_type(response, "application/json");
        http_response_set_body(response, body);
        return;
    }

    // Update user in database
    pthread_mutex_lock(&db_mutex);
    int result = db_update_user(&app_db, user_id, name, email);
    pthread_mutex_unlock(&db_mutex);

    if (result > 0)
    {
        // Success - user updated
        snprintf(body, sizeof(body),
                 "{\n"
                 "  \"message\": \"User updated successfully\",\n"
                 "  \"id\": %d,\n"
                 "  \"name\": \"%s\",\n"
                 "  \"email\": \"%s\"\n"
                 "}",
                 user_id, name, email);

        http_response_set_status(response, HTTP_200_OK);
        printf("User %d updated successfully\n", user_id);
    }
    else if (result == 0)
    {
        // User not found
        snprintf(body, sizeof(body),
                 "{\n"
                 "  \"error\": \"Not Found\",\n"
                 "  \"message\": \"User with ID %d not found\"\n"
                 "}",
                 user_id);

        http_response_set_status(response, HTTP_404_NOT_FOUND);
        printf("User %d not found for update\n", user_id);
    }
    else
    {
        // Database error
        snprintf(body, sizeof(body),
                 "{\n"
                 "  \"error\": \"Internal Server Error\",\n"
                 "  \"message\": \"Database error occurred while updating user\"\n"
                 "}");

        http_response_set_status(response, HTTP_500_INTERNAL_ERROR);
        printf("Database error while updating user %d\n", user_id);
    }

    http_response_set_content_type(response, "application/json");
    http_response_set_body(response, body);
}

void route_partial_update_user(const HTTP_REQUEST *request, HTTP_RESPONSE *response)
{
    char body[1024];
    char current_user_json[512];
    char name[256] = {0};
    char email[256] = {0};

    // Extract user ID from URL path "/api/users/123"
    const char *user_id_str = get_url_param(request, "id");
    int user_id = atoi(user_id_str);

    if (user_id <= 0)
    {
        snprintf(body, sizeof(body),
                 "{\n"
                 "  \"error\": \"Bad Request\",\n"
                 "  \"message\": \"Invalid user ID\"\n"
                 "}");

        http_response_set_status(response, HTTP_400_BAD_REQUEST);
        http_response_set_content_type(response, "application/json");
        http_response_set_body(response, body);
        return;
    }

    // Check if request body exists
    if (!request->body || request->content_length <= 0)
    {
        snprintf(body, sizeof(body),
                 "{\n"
                 "  \"error\": \"Bad Request\",\n"
                 "  \"message\": \"Request body is required for partial user update\"\n"
                 "}");

        http_response_set_status(response, HTTP_400_BAD_REQUEST);
        http_response_set_content_type(response, "application/json");
        http_response_set_body(response, body);
        return;
    }

    printf("Partially updating user %d with data: %s\n", user_id, request->body);

    // Get current user data
    pthread_mutex_lock(&db_mutex);
    int user_exists = db_get_user_by_id(&app_db, user_id, current_user_json, sizeof(current_user_json));
    pthread_mutex_unlock(&db_mutex);

    if (user_exists < 0)
    {
        snprintf(body, sizeof(body),
                 "{\n"
                 "  \"error\": \"Internal Server Error\",\n"
                 "  \"message\": \"Database error occurred\"\n"
                 "}");

        http_response_set_status(response, HTTP_500_INTERNAL_ERROR);
        http_response_set_content_type(response, "application/json");
        http_response_set_body(response, body);
        return;
    }

    if (user_exists == 0)
    {
        snprintf(body, sizeof(body),
                 "{\n"
                 "  \"error\": \"Not Found\",\n"
                 "  \"message\": \"User with ID %d not found\"\n"
                 "}",
                 user_id);

        http_response_set_status(response, HTTP_404_NOT_FOUND);
        http_response_set_content_type(response, "application/json");
        http_response_set_body(response, body);
        return;
    }

    // Extract current values
    char current_name[256] = {0};
    char current_email[256] = {0};

    parse_json_field(current_user_json, "name", current_name, sizeof(current_name));
    parse_json_field(current_user_json, "email", current_email, sizeof(current_email));

    // Start with current values
    strcpy(name, current_name);
    strcpy(email, current_email);

    // Apply patch values if present
    char patched_name[256] = {0};
    char patched_email[256] = {0};

    if (parse_json_field(request->body, "name", patched_name, sizeof(patched_name)) == 0)
    {
        strcpy(name, patched_name);
    }

    if (parse_json_field(request->body, "email", patched_email, sizeof(patched_email)) == 0)
    {
        strcpy(email, patched_email);
    }

    // Validate the final values
    if (!is_valid_name(name) || !is_valid_email(email))
    {
        snprintf(body, sizeof(body),
                 "{\n"
                 "  \"error\": \"Bad Request\",\n"
                 "  \"message\": \"Name and email must be valid (3-50 characters, alphanumeric and underscores only for name)\"\n"
                 "}");

        http_response_set_status(response, HTTP_400_BAD_REQUEST);
        http_response_set_content_type(response, "application/json");
        http_response_set_body(response, body);
        return;
    }

    // Update user in database
    pthread_mutex_lock(&db_mutex);
    int result = db_update_user(&app_db, user_id, name, email);
    pthread_mutex_unlock(&db_mutex);

    if (result > 0)
    {
        snprintf(body, sizeof(body),
                 "{\n"
                 "  \"message\": \"User partially updated successfully\",\n"
                 "  \"id\": %d,\n"
                 "  \"name\": \"%s\",\n"
                 "  \"email\": \"%s\"\n"
                 "}",
                 user_id, name, email);

        http_response_set_status(response, HTTP_200_OK);
        printf("User %d partially updated successfully\n", user_id);
    }
    else if (result == 0)
    {
        snprintf(body, sizeof(body),
                 "{\n"
                 "  \"error\": \"Not Found\",\n"
                 "  \"message\": \"User with ID %d not found\"\n"
                 "}",
                 user_id);

        http_response_set_status(response, HTTP_404_NOT_FOUND);
    }
    else
    {
        snprintf(body, sizeof(body),
                 "{\n"
                 "  \"error\": \"Internal Server Error\",\n"
                 "  \"message\": \"Database error occurred while updating user\"\n"
                 "}");

        http_response_set_status(response, HTTP_500_INTERNAL_ERROR);
    }

    http_response_set_content_type(response, "application/json");
    http_response_set_body(response, body);
}

void route_delete_user(const HTTP_REQUEST *request, HTTP_RESPONSE *response)
{
    char body[512];
    const char *user_id_str = get_url_param(request, "id");
    int user_id = atoi(user_id_str);

    if (user_id <= 0)
    {
        snprintf(body, sizeof(body),
                 "{\n"
                 "  \"error\": \"Bad Request\",\n"
                 "  \"message\": \"Invalid user ID\"\n"
                 "}");

        http_response_set_status(response, HTTP_400_BAD_REQUEST);
        http_response_set_content_type(response, "application/json");
        http_response_set_body(response, body);
        return;
    }

    printf("Deleting user %d\n", user_id);

    // Delete user from database
    pthread_mutex_lock(&db_mutex);
    int result = db_delete_user(&app_db, user_id);
    pthread_mutex_unlock(&db_mutex);

    if (result > 0)
    {
        // Success - user deleted
        snprintf(body, sizeof(body),
                 "{\n"
                 "  \"message\": \"User deleted successfully\",\n"
                 "  \"deleted_user_id\": %d\n"
                 "}",
                 user_id);

        http_response_set_status(response, HTTP_200_OK);
        printf("User %d deleted successfully\n", user_id);
    }
    else if (result == 0)
    {
        // User not found
        snprintf(body, sizeof(body),
                 "{\n"
                 "  \"error\": \"Not Found\",\n"
                 "  \"message\": \"User with ID %d not found\"\n"
                 "}",
                 user_id);

        http_response_set_status(response, HTTP_404_NOT_FOUND);
        printf("User %d not found for deletion\n", user_id);
    }
    else
    {
        // Database error
        snprintf(body, sizeof(body),
                 "{\n"
                 "  \"error\": \"Internal Server Error\",\n"
                 "  \"message\": \"Database error occurred while deleting user\"\n"
                 "}");

        http_response_set_status(response, HTTP_500_INTERNAL_ERROR);
        printf("Database error while deleting user %d\n", user_id);
    }

    http_response_set_content_type(response, "application/json");
    http_response_set_body(response, body);
}

void route_login(const HTTP_REQUEST *request, HTTP_RESPONSE *response)
{
    char body[1024];

    if (request->body && request->content_length > 0)
    {
        printf("Login attempt with data: %s\n", request->body);

        snprintf(body, sizeof(body),
                 "{\n"
                 "  \"message\": \"Login successful\",\n"
                 "  \"token\": \"fake_jwt_token_123\",\n"
                 "  \"user\": {\"id\": 1, \"username\": \"authenticated_user\"}\n"
                 "}");

        http_response_set_status(response, HTTP_200_OK);
    }
    else
    {
        snprintf(body, sizeof(body),
                 "{\n"
                 "  \"error\": \"Invalid credentials\",\n"
                 "  \"message\": \"Username and password are required\"\n"
                 "}");

        http_response_set_status(response, HTTP_401_UNAUTHORIZED);
    }

    http_response_set_content_type(response, "application/json");
    http_response_set_body(response, body);
}

void route_not_found(const HTTP_REQUEST *request, HTTP_RESPONSE *response)
{
    (void)request;

    const char *body =
        "<!DOCTYPE html>\n"
        "<html>\n"
        "<head><title>404 Not Found</title></head>\n"
        "<body>\n"
        "<h1>404 - Page Not Found</h1>\n"
        "<p>The requested resource was not found on this server.</p>\n"
        "<p><a href=\"/\">Go back to home</a></p>\n"
        "</body>\n"
        "</html>\n";

    http_response_set_status(response, HTTP_404_NOT_FOUND);
    http_response_set_body(response, body);
}