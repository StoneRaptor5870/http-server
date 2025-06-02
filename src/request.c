#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/request.h"

void http_request_init(HTTP_REQUEST *request)
{
    memset(request->method, 0, sizeof(request->method));
    memset(request->path, 0, sizeof(request->path));
    memset(request->version, 0, sizeof(request->version));
    request->body = NULL;
    request->content_length = 0;
}

int http_request_parse(const char *raw_request, HTTP_REQUEST *request)
{
    if (!raw_request || !request)
    {
        return -1;
    }

    http_request_init(request);

    // Parse the request line (method, path, version)
    int parsed = sscanf(raw_request, "%15s %255s %15s",
                        request->method, request->path, request->version);

    if (parsed < 2)
    {
        printf("Failed to parse HTTP request line\n");
        return -1;
    }

    // If version wasn't provided, assume HTTP/1.1
    if (parsed == 2)
    {
        strcpy(request->version, "HTTP/1.1");
    }

    // Look for Content-Length header
    const char *content_length_header = strstr(raw_request, "Content-Length:");
    if (content_length_header)
    {
        sscanf(content_length_header, "Content-Length: %d", &request->content_length);
    }

    // Find the body (after \r\n\r\n)
    const char *body_start = strstr(raw_request, "\r\n\r\n");
    if (body_start && request->content_length > 0)
    {
        body_start += 4; // Skip \r\n\r\n
        request->body = malloc(request->content_length + 1);
        if (request->body)
        {
            strncpy(request->body, body_start, request->content_length);
            request->body[request->content_length] = '\0';
        }
    }

    return 0;
}

void http_request_cleanup(HTTP_REQUEST *request)
{
    if (request && request->body)
    {
        free(request->body);
        request->body = NULL;
    }
}

void http_request_print(const HTTP_REQUEST *request)
{
    printf("Method: %s\n", request->method);
    printf("Path: %s\n", request->path);
    printf("Version: %s\n", request->version);
    printf("Content-Length: %d\n", request->content_length);
    if (request->body)
    {
        printf("Body: %s\n", request->body);
    }
}