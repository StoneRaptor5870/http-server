#ifndef RESPONSE_H
#define RESPONSE_H

#include "config.h"

typedef enum
{
    HTTP_200_OK,
    HTTP_201_CREATED,
    HTTP_400_BAD_REQUEST,
    HTTP_401_UNAUTHORIZED,
    HTTP_404_NOT_FOUND,
    HTTP_405_METHOD_NOT_ALLOWED,
    HTTP_500_INTERNAL_ERROR,
    HTTP_409_CONFLICT
} HTTP_STATUS;

typedef struct
{
    HTTP_STATUS status;
    char content_type[64];
    char *body;
    int body_length;
} HTTP_RESPONSE;

// HTTP response functions
void http_response_init(HTTP_RESPONSE *response);
void http_response_set_status(HTTP_RESPONSE *response, HTTP_STATUS status);
void http_response_set_content_type(HTTP_RESPONSE *response, const char *content_type);
void http_response_set_body(HTTP_RESPONSE *response, const char *body);
int http_response_build(const HTTP_RESPONSE *response, char *buffer, int buffer_size);
void http_response_cleanup(HTTP_RESPONSE *response);

#endif // RESPONSE_H