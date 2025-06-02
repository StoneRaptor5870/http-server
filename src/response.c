#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/response.h"

static const char *get_status_text(HTTP_STATUS status)
{
    switch (status)
    {
    case HTTP_200_OK:
        return "200 OK";
    case HTTP_404_NOT_FOUND:
        return "404 Not Found";
    case HTTP_500_INTERNAL_ERROR:
        return "500 Internal Server Error";
    default:
        return "200 OK";
    }
}

void http_response_init(HTTP_RESPONSE *response)
{
    response->status = HTTP_200_OK;
    strcpy(response->content_type, "text/html");
    response->body = NULL;
    response->body_length = 0;
}

void http_response_set_status(HTTP_RESPONSE *response, HTTP_STATUS status)
{
    response->status = status;
}

void http_response_set_content_type(HTTP_RESPONSE *response, const char *content_type)
{
    strncpy(response->content_type, content_type, sizeof(response->content_type) - 1);
    response->content_type[sizeof(response->content_type) - 1] = '\0';
}

void http_response_set_body(HTTP_RESPONSE *response, const char *body)
{
    if (response->body)
    {
        free(response->body);
    }

    if (body)
    {
        response->body_length = strlen(body);
        response->body = malloc(response->body_length + 1);
        if (response->body)
        {
            strcpy(response->body, body);
        }
    }
    else
    {
        response->body = NULL;
        response->body_length = 0;
    }
}

int http_response_build(const HTTP_RESPONSE *response, char *buffer, int buffer_size)
{
    const char *status_text = get_status_text(response->status);

    int written = snprintf(buffer, buffer_size,
                           "HTTP/1.1 %s\r\n"
                           "Content-Type: %s\r\n"
                           "Content-Length: %d\r\n"
                           "Connection: close\r\n"
                           "\r\n"
                           "%s",
                           status_text,
                           response->content_type,
                           response->body_length,
                           response->body ? response->body : "");

    return (written < buffer_size) ? written : -1;
}

void http_response_cleanup(HTTP_RESPONSE *response)
{
    if (response && response->body)
    {
        free(response->body);
        response->body = NULL;
        response->body_length = 0;
    }
}