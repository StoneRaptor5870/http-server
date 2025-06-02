#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/config.h"
#include "../include/routes.h"

void route_home(const HTTP_REQUEST *request, HTTP_RESPONSE *response)
{
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
             "<h2>API Test Form</h2>\n"
             "<form id=\"userForm\">\n"
             "  <input type=\"text\" id=\"username\" placeholder=\"Username\" required>\n"
             "  <input type=\"email\" id=\"email\" placeholder=\"Email\" required>\n"
             "  <button type=\"button\" onclick=\"createUser()\">Create User (POST)</button>\n"
             "  <button type=\"button\" onclick=\"getUsers()\">Get Users (GET)</button>\n"
             "</form>\n"
             "<div id=\"result\"></div>\n"
             "<script>\n"
             "async function createUser() {\n"
             "  const username = document.getElementById('username').value;\n"
             "  const email = document.getElementById('email').value;\n"
             "  const response = await fetch('/api/users', {\n"
             "    method: 'POST',\n"
             "    headers: {'Content-Type': 'application/json'},\n"
             "    body: JSON.stringify({username, email})\n"
             "  });\n"
             "  const result = await response.text();\n"
             "  document.getElementById('result').innerHTML = result;\n"
             "}\n"
             "async function getUsers() {\n"
             "  const response = await fetch('/api/users');\n"
             "  const result = await response.text();\n"
             "  document.getElementById('result').innerHTML = result;\n"
             "}\n"
             "</script>\n"
             "</body>\n"
             "</html>\n",
             DEFAULT_PORT, request->path);

    http_response_set_status(response, HTTP_200_OK);
    http_response_set_body(response, body);
}

void route_about(const HTTP_REQUEST *request, HTTP_RESPONSE *response)
{
    (void)request;

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
    http_response_set_body(response, body);
}

// API Routes
void route_get_users(const HTTP_REQUEST *request, HTTP_RESPONSE *response)
{
    (void)request;

    const char *json_body =
        "{\n"
        "  \"users\": [\n"
        "    {\"id\": 1, \"username\": \"john_doe\", \"email\": \"john@example.com\"},\n"
        "    {\"id\": 2, \"username\": \"jane_smith\", \"email\": \"jane@example.com\"}\n"
        "  ]\n"
        "}";

    http_response_set_status(response, HTTP_200_OK);
    http_response_set_content_type(response, "application/json");
    http_response_set_body(response, json_body);
}

void route_create_user(const HTTP_REQUEST *request, HTTP_RESPONSE *response)
{
    char body[1024];

    if (request->body && request->content_length > 0)
    {
        printf("Creating user with data: %s\n", request->body);

        snprintf(body, sizeof(body),
                 "{\n"
                 "  \"message\": \"User created successfully\",\n"
                 "  \"id\": 3,\n"
                 "  \"received_data\": %s\n"
                 "}",
                 request->body);

        http_response_set_status(response, HTTP_201_CREATED);
    }
    else
    {
        snprintf(body, sizeof(body),
                 "{\n"
                 "  \"error\": \"No data provided\",\n"
                 "  \"message\": \"Request body is required for user creation\"\n"
                 "}");

        http_response_set_status(response, HTTP_400_BAD_REQUEST);
    }

    http_response_set_content_type(response, "application/json");
    http_response_set_body(response, body);
}

void route_update_user(const HTTP_REQUEST *request, HTTP_RESPONSE *response)
{
    char body[1024];
    const char *user_id = request->path + 11; // Skip "/api/users/"

    if (request->body && request->content_length > 0)
    {
        printf("Updating user %s with data: %s\n", user_id, request->body);

        snprintf(body, sizeof(body),
                 "{\n"
                 "  \"message\": \"User updated successfully\",\n"
                 "  \"user_id\": \"%s\",\n"
                 "  \"updated_data\": %s\n"
                 "}",
                 user_id, request->body);

        http_response_set_status(response, HTTP_200_OK);
    }
    else
    {
        snprintf(body, sizeof(body),
                 "{\n"
                 "  \"error\": \"No data provided\",\n"
                 "  \"message\": \"Request body is required for user update\"\n"
                 "}");

        http_response_set_status(response, HTTP_400_BAD_REQUEST);
    }

    http_response_set_content_type(response, "application/json");
    http_response_set_body(response, body);
}

void route_partial_update_user(const HTTP_REQUEST *request, HTTP_RESPONSE *response)
{
    char body[1024];
    const char *user_id = request->path + 11; // Skip "/api/users/"

    if (request->body && request->content_length > 0)
    {
        printf("Partially updating user %s with data: %s\n", user_id, request->body);

        snprintf(body, sizeof(body),
                 "{\n"
                 "  \"message\": \"User partially updated successfully\",\n"
                 "  \"user_id\": \"%s\",\n"
                 "  \"patched_data\": %s\n"
                 "}",
                 user_id, request->body);

        http_response_set_status(response, HTTP_200_OK);
    }
    else
    {
        snprintf(body, sizeof(body),
                 "{\n"
                 "  \"error\": \"No data provided\",\n"
                 "  \"message\": \"Request body is required for partial user update\"\n"
                 "}");

        http_response_set_status(response, HTTP_400_BAD_REQUEST);
    }

    http_response_set_content_type(response, "application/json");
    http_response_set_body(response, body);
}

void route_delete_user(const HTTP_REQUEST *request, HTTP_RESPONSE *response)
{
    char body[512];
    const char *user_id = request->path + 11; // Skip "/api/users/"

    printf("Deleting user %s\n", user_id);

    snprintf(body, sizeof(body),
             "{\n"
             "  \"message\": \"User deleted successfully\",\n"
             "  \"deleted_user_id\": \"%s\"\n"
             "}",
             user_id);

    http_response_set_status(response, HTTP_200_OK);
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