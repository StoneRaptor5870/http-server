#ifndef REQUEST_H
#define REQUEST_H

#include "config.h"

typedef struct
{
    char method[MAX_METHOD_LENGTH];
    char path[MAX_PATH_LENGTH];
    char version[16];
    char *body;
    int content_length;
} HTTP_REQUEST;

// HTTP request functions
int http_request_parse(const char *raw_request, HTTP_REQUEST *request);
void http_request_init(HTTP_REQUEST *request);
void http_request_cleanup(HTTP_REQUEST *request);
void http_request_print(const HTTP_REQUEST *request);

#endif // REQUEST_H