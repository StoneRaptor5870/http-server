#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/time.h>
#include "../include/config.h"
#include "../include/handler.h"
#include "../include/routes.h"
#include "../include/file.h"

void handle_http_request(int client_socket)
{
    char buffer[BUFFER_SIZE];
    HTTP_REQUEST request;
    HTTP_RESPONSE response;
    char response_buffer[MAX_RESPONSE_SIZE];

    struct timeval timeout = {2, 0}; // 2 seconds
    setsockopt(client_socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    // Read the request
    int bytes_received = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);
    if (bytes_received <= 0)
    {
        // perror("recv failed");
        close(client_socket);  // Doing this to ignore empty connections from client (browser)
        return;
    }

    buffer[bytes_received] = '\0';
    printf("Raw request (%d bytes): '%s'\n", bytes_received, buffer);

    // Parse the request
    if (http_request_parse(buffer, &request) < 0)
    {
        printf("Failed to parse HTTP request\n");
        return;
    }

    http_request_print(&request);

    // Initialize response
    http_response_init(&response);

    // Route based on method and path
    if (strcmp(request.method, "GET") == 0)
    {
        handle_get_request(&request, &response);
    }
    else if (strcmp(request.method, "POST") == 0)
    {
        handle_post_request(&request, &response);
    }
    else if (strcmp(request.method, "PUT") == 0)
    {
        handle_put_request(&request, &response);
    }
    else if (strcmp(request.method, "PATCH") == 0)
    {
        handle_patch_request(&request, &response);
    }
    else if (strcmp(request.method, "DELETE") == 0)
    {
        handle_delete_request(&request, &response);
    }
    else
    {
        // Method not allowed
        route_method_not_allowed(&request, &response);
    }

    // Build and send response
    if (http_response_build(&response, response_buffer, sizeof(response_buffer)) > 0)
    {
        send(client_socket, response_buffer, strlen(response_buffer), 0);
        printf("Response sent\n\n");
    }
    else
    {
        printf("Failed to build response\n");
    }

    // Cleanup
    http_request_cleanup(&request);
    http_response_cleanup(&response);
}

void handle_get_request(HTTP_REQUEST *request, HTTP_RESPONSE *response)
{
    // Check for exact matches first
    if (strcmp(request->clean_path, "/") == 0 || strcmp(request->clean_path, "/index.html") == 0)
    {
        route_home(request, response);
    }
    else if (strcmp(request->clean_path, "/css/style.css") == 0)
    {
        route_get_css(request, response);
    }
    else if (strcmp(request->clean_path, "/js/app.js") == 0)
    {
        route_get_js(request, response);
    }
    else if (strcmp(request->clean_path, "/about") == 0 || strcmp(request->clean_path, "/about.html") == 0)
    {
        route_about(request, response);
    }
    else if (strcmp(request->clean_path, "/api/users") == 0)
    {
        route_get_users(request, response);
    }
    // Check for parameterized routes
    else if (match_path_pattern("/api/users/{id}", request->clean_path))
    {
        extract_and_store_url_params(request, "/api/users/{id}");
        http_request_print(request);
        route_get_user_by_id(request, response);
    }
    else
    {
        route_not_found(request, response);
    }
}

void handle_post_request(HTTP_REQUEST *request, HTTP_RESPONSE *response)
{
    if (strcmp(request->path, "/api/users") == 0)
    {
        route_create_user(request, response);
    }
    else if (strcmp(request->path, "/api/login") == 0)
    {
        route_login(request, response);
    }
    else
    {
        route_not_found(request, response);
    }
}

void handle_put_request(HTTP_REQUEST *request, HTTP_RESPONSE *response)
{
    if (match_path_pattern("/api/users/{id}", request->clean_path))
    {
        extract_and_store_url_params(request, "/api/users/{id}");
        route_update_user(request, response);
    }
    else
    {
        route_not_found(request, response);
    }
}

void handle_patch_request(HTTP_REQUEST *request, HTTP_RESPONSE *response)
{
    if (match_path_pattern("/api/users/{id}", request->clean_path))
    {
        extract_and_store_url_params(request, "/api/users/{id}");
        route_partial_update_user(request, response);
    }
    else
    {
        route_not_found(request, response);
    }
}

void handle_delete_request(HTTP_REQUEST *request, HTTP_RESPONSE *response)
{
    if (match_path_pattern("/api/users/{id}", request->clean_path))
    {
        extract_and_store_url_params(request, "/api/users/{id}");
        route_delete_user(request, response);
    }
    else
    {
        route_not_found(request, response);
    }
}

void route_method_not_allowed(const HTTP_REQUEST *request, HTTP_RESPONSE *response)
{
    (void)request;

    const char *body =
        "{\n"
        "  \"error\": \"Method Not Allowed\",\n"
        "  \"message\": \"The HTTP method is not supported for this endpoint\"\n"
        "}";

    http_response_set_status(response, HTTP_405_METHOD_NOT_ALLOWED);
    http_response_set_content_type(response, "application/json");
    http_response_set_body(response, body);
}