#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include "../include/config.h"
#include "../include/handler.h"

void handle_http_request(int client_socket)
{
    char buffer[BUFFER_SIZE];
    HTTP_REQUEST request;
    HTTP_RESPONSE response;
    char response_buffer[MAX_RESPONSE_SIZE];

    // Read the request
    int bytes_received = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);
    if (bytes_received < 0)
    {
        perror("recv failed");
        return;
    }

    buffer[bytes_received] = '\0';
    printf("Received request:\n%s\n", buffer);

    // Parse the request
    if (http_request_parse(buffer, &request) < 0)
    {
        printf("Failed to parse HTTP request\n");
        return;
    }

    http_request_print(&request);

    // Initialize response
    http_response_init(&response);

    // Route the request
    if (strcmp(request.path, "/") == 0 || strcmp(request.path, "/index.html") == 0)
    {
        route_home(&request, &response);
    }
    else if (strcmp(request.path, "/about") == 0)
    {
        route_about(&request, &response);
    }
    else
    {
        route_not_found(&request, &response);
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

void route_home(const HTTP_REQUEST *request, HTTP_RESPONSE *response)
{
    char body[1024];
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
        "<li>HTTP protocol handling</li>\n"
        "<li>Request parsing and response generation</li>\n"
        "</ul>\n"
        "<p><a href=\"/\">Go back to home</a></p>\n"
        "</body>\n"
        "</html>\n";

    http_response_set_status(response, HTTP_200_OK);
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