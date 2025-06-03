#ifndef REQUEST_H
#define REQUEST_H

#include <stdbool.h>
#include "config.h"

typedef struct
{
    char key[64];
    char value[256];
} QueryParam;

typedef struct
{
    char key[64];
    char value[256];
} UrlParam;

typedef struct
{
    char method[MAX_METHOD_LENGTH];
    char path[MAX_PATH_LENGTH];
    char clean_path[MAX_PATH_LENGTH];
    char query_string[512];
    char version[16];
    char *body;
    int content_length;

    // Query parameters (?key=value&key2=value2)
    QueryParam query_params[MAX_QUERY_PARAMS];
    int query_param_count;

    // URL parameters (extracted from path like /api/users/{id})
    UrlParam url_params[MAX_URL_PARAMS];
    int url_param_count;
} HTTP_REQUEST;

// HTTP request functions
int http_request_parse(const char *raw_request, HTTP_REQUEST *request);
void http_request_init(HTTP_REQUEST *request);
void http_request_cleanup(HTTP_REQUEST *request);
void http_request_print(const HTTP_REQUEST *request);

// Parameter helper functions
const char *get_query_param(const HTTP_REQUEST *request, const char *key);
const char *get_url_param(const HTTP_REQUEST *request, const char *key);
int parse_query_string(const char *query_string, QueryParam *params, int max_params);
int extract_and_store_url_params(HTTP_REQUEST *request, const char *path_pattern);
bool match_path_pattern(const char *pattern, const char *path);

// URL decoding
int url_decode(char *dst, size_t dst_size, const char *src);

#endif // REQUEST_H