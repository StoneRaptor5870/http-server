#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "../include/request.h"

static void safe_strncpy(char *dest, const char *src, size_t dest_size)
{
    if (dest_size == 0)
        return;
    strncpy(dest, src, dest_size - 1);
    dest[dest_size - 1] = '\0';
}

static bool is_safe_path(const char *path)
{
    if (!path)
        return false;

    // Check for directory traversal patterns
    if (strstr(path, "../") || strstr(path, "..\\") ||
        strstr(path, "/..") || strstr(path, "\\.."))
    {
        return false;
    }

    // Check for null bytes
    if (strlen(path) != strcspn(path, "\0"))
    {
        return false;
    }

    return true;
}

void http_request_init(HTTP_REQUEST *request)
{
    if (!request)
        return;

    memset(request->method, 0, sizeof(request->method));
    memset(request->path, 0, sizeof(request->path));
    memset(request->clean_path, 0, sizeof(request->clean_path));
    memset(request->query_string, 0, sizeof(request->query_string));
    memset(request->version, 0, sizeof(request->version));
    request->body = NULL;
    request->content_length = 0;
    request->query_param_count = 0;
    request->url_param_count = 0;

    // Initialize parameter arrays
    for (int i = 0; i < MAX_QUERY_PARAMS; i++)
    {
        memset(request->query_params[i].key, 0, sizeof(request->query_params[i].key));
        memset(request->query_params[i].value, 0, sizeof(request->query_params[i].value));
    }

    for (int i = 0; i < MAX_URL_PARAMS; i++)
    {
        memset(request->url_params[i].key, 0, sizeof(request->url_params[i].key));
        memset(request->url_params[i].value, 0, sizeof(request->url_params[i].value));
    }
}

int url_decode(char *dst, size_t dst_size, const char *src)
{
    if (!dst || !src || dst_size == 0)
        return -1;

    size_t dst_idx = 0;
    const char *src_ptr = src;

    while (*src_ptr && dst_idx < dst_size - 1)
    {
        if (*src_ptr == '%' && src_ptr[1] && src_ptr[2] &&
            isxdigit(src_ptr[1]) && isxdigit(src_ptr[2]))
        {

            char hex_str[3] = {src_ptr[1], src_ptr[2], '\0'};
            char *endptr;
            long val = strtol(hex_str, &endptr, 16);

            if (*endptr == '\0' && val >= 0 && val <= 255)
            {
                dst[dst_idx++] = (char)val;
                src_ptr += 3;
            }
            else
            {
                dst[dst_idx++] = *src_ptr++;
            }
        }
        else if (*src_ptr == '+')
        {
            dst[dst_idx++] = ' ';
            src_ptr++;
        }
        else
        {
            dst[dst_idx++] = *src_ptr++;
        }
    }

    dst[dst_idx] = '\0';
    return (*src_ptr == '\0') ? 0 : -1; // Return -1 if source wasn't fully processed
}

// Parse query string into key-value pairs
int parse_query_string(const char *query_string, QueryParam *params, int max_params)
{
    if (!query_string || !params || max_params <= 0)
    {
        return 0;
    }

    size_t query_len = strlen(query_string);
    if (query_len == 0 || query_len > MAX_HEADER_LENGTH)
    {
        return 0;
    }

    char *query_copy = malloc(query_len + 1);
    if (!query_copy)
    {
        return 0;
    }

    safe_strncpy(query_copy, query_string, query_len + 1);

    char *saveptr;
    char *token = strtok_r(query_copy, "&", &saveptr);
    int count = 0;

    while (token && count < max_params)
    {
        char *equals = strchr(token, '=');
        if (equals && equals != token)
        { // Ensure key is not empty
            *equals = '\0';
            char *key = token;
            char *value = equals + 1;

            // Validate lengths before decoding
            if (strlen(key) < MAX_PARAM_KEY_LENGTH &&
                strlen(value) < MAX_PARAM_VALUE_LENGTH)
            {

                // URL decode key and value with bounds checking
                if (url_decode(params[count].key, sizeof(params[count].key), key) == 0 &&
                    url_decode(params[count].value, sizeof(params[count].value), value) == 0)
                {
                    count++;
                }
            }
        }
        else if (!equals && strlen(token) < MAX_PARAM_KEY_LENGTH)
        {
            // Handle keys without values
            if (url_decode(params[count].key, sizeof(params[count].key), token) == 0)
            {
                params[count].value[0] = '\0';
                count++;
            }
        }
        token = strtok_r(NULL, "&", &saveptr);
    }

    free(query_copy);
    return count;
}

int http_request_parse(const char *raw_request, HTTP_REQUEST *request)
{
    if (!raw_request || !request)
    {
        return -1;
    }

    // Check request size limit
    size_t request_len = strlen(raw_request);
    if (request_len > MAX_REQUEST_SIZE)
    {
        printf("Request too large: %zu bytes\n", request_len);
        return -1;
    }

    http_request_init(request);

    char method[16] = {0};
    char path[MAX_PATH_LENGTH] = {0};
    char version[16] = {0};

    // Parse the request line with bounds checking
    int parsed = sscanf(raw_request, "%15s %255s %15s", method, path, version);

    if (parsed < 2)
    {
        printf("Failed to parse HTTP request line\n");
        return -1;
    }

    // Validate and copy parsed values
    safe_strncpy(request->method, method, sizeof(request->method));
    safe_strncpy(request->path, path, sizeof(request->path));

    if (parsed == 2)
    {
        safe_strncpy(request->version, "HTTP/1.1", sizeof(request->version));
    }
    else
    {
        safe_strncpy(request->version, version, sizeof(request->version));
    }

    if (!is_safe_path(request->path))
    {
        printf("Unsafe path detected: %s\n", request->path);
        return -1;
    }

    // Split path and query string
    char *question_mark = strchr(request->path, '?');
    if (question_mark)
    {
        // Validate query string length
        size_t query_len = strlen(question_mark + 1);
        if (query_len >= sizeof(request->query_string))
        {
            printf("Query string too long\n");
            return -1;
        }

        safe_strncpy(request->query_string, question_mark + 1, sizeof(request->query_string));

        size_t path_len = question_mark - request->path;
        if (path_len >= sizeof(request->clean_path))
        {
            printf("Path too long\n");
            return -1;
        }

        strncpy(request->clean_path, request->path, path_len);
        request->clean_path[path_len] = '\0';

        // Parse query parameters
        request->query_param_count = parse_query_string(request->query_string,
                                                        request->query_params,
                                                        MAX_QUERY_PARAMS);
    }
    else
    {
        // No query string, clean path is same as original path
        safe_strncpy(request->clean_path, request->path, sizeof(request->clean_path));
        request->query_string[0] = '\0';
        request->query_param_count = 0;
    }

    // Look for Content-Length header
    const char *content_length_header = strstr(raw_request, "Content-Length:");
    if (content_length_header)
    {
        char *endptr;
        long content_length = strtol(content_length_header + 15, &endptr, 10);

        // Validate content length
        if (content_length >= 0 && content_length <= MAX_REQUEST_SIZE &&
            (*endptr == '\r' || *endptr == '\n' || isspace(*endptr)))
        {
            request->content_length = (int)content_length;
        }
        else
        {
            printf("Invalid Content-Length header\n");
            return -1;
        }
    }

    // Find and process body if present
    const char *body_start = strstr(raw_request, "\r\n\r\n");
    if (body_start && request->content_length > 0)
    {
        body_start += 4; // Skip \r\n\r\n

        size_t remaining = request_len - (body_start - raw_request);
        if (remaining < (size_t)request->content_length)
        {
            printf("Incomplete request body\n");
            return -1;
        }

        request->body = malloc(request->content_length + 1);
        if (!request->body)
        {
            printf("Failed to allocate memory for request body\n");
            return -1;
        }

        memcpy(request->body, body_start, request->content_length);
        request->body[request->content_length] = '\0';
    }

    return 0;
}

// Get query parameter value by key
const char *get_query_param(const HTTP_REQUEST *request, const char *key)
{
    if (!request || !key || strlen(key) == 0)
    {
        return NULL;
    }

    for (int i = 0; i < request->query_param_count; i++)
    {
        if (strcmp(request->query_params[i].key, key) == 0)
        {
            return request->query_params[i].value;
        }
    }
    return NULL;
}

// Get URL parameter value by key
const char *get_url_param(const HTTP_REQUEST *request, const char *key)
{
    if (!request || !key || strlen(key) == 0)
    {
        return NULL;
    }

    for (int i = 0; i < request->url_param_count; i++)
    {
        if (strcmp(request->url_params[i].key, key) == 0)
        {
            return request->url_params[i].value;
        }
    }
    return NULL;
}

bool match_path_pattern(const char *pattern, const char *path)
{
    if (!pattern || !path)
    {
        return false;
    }

    size_t pattern_len = strlen(pattern);
    size_t path_len = strlen(path);

    if (pattern_len >= 512 || path_len >= 512)
    {
        return false; // Reject overly long paths
    }

    char pattern_copy[512], path_copy[512];
    safe_strncpy(pattern_copy, pattern, sizeof(pattern_copy));
    safe_strncpy(path_copy, path, sizeof(path_copy));

    char *pattern_saveptr, *path_saveptr;
    char *pattern_token = strtok_r(pattern_copy, "/", &pattern_saveptr);
    char *path_token = strtok_r(path_copy, "/", &path_saveptr);

    while (pattern_token && path_token)
    {
        size_t token_len = strlen(pattern_token);

        // Check if it's a parameter (enclosed in braces)
        if (token_len >= 3 && pattern_token[0] == '{' &&
            pattern_token[token_len - 1] == '}')
        {
            // Parameter matches any non-empty token
            if (strlen(path_token) == 0)
            {
                return false;
            }
        }
        else
        {
            // Exact match required
            if (strcmp(pattern_token, path_token) != 0)
            {
                return false;
            }
        }

        pattern_token = strtok_r(NULL, "/", &pattern_saveptr);
        path_token = strtok_r(NULL, "/", &path_saveptr);
    }

    return pattern_token == NULL && path_token == NULL;
}

int extract_and_store_url_params(HTTP_REQUEST *request, const char *path_pattern)
{
    if (!request || !path_pattern)
    {
        return -1;
    }

    size_t pattern_len = strlen(path_pattern);
    size_t path_len = strlen(request->clean_path);

    if (pattern_len >= 512 || path_len >= 512)
    {
        return -1;
    }

    char pattern_copy[512], path_copy[512];
    safe_strncpy(pattern_copy, path_pattern, sizeof(pattern_copy));
    safe_strncpy(path_copy, request->clean_path, sizeof(path_copy));

    char *pattern_saveptr, *path_saveptr;
    char *pattern_token = strtok_r(pattern_copy, "/", &pattern_saveptr);
    char *path_token = strtok_r(path_copy, "/", &path_saveptr);

    request->url_param_count = 0;

    while (pattern_token && path_token &&
           request->url_param_count < MAX_URL_PARAMS)
    {

        size_t token_len = strlen(pattern_token);

        // Check if pattern token is a parameter
        if (token_len >= 3 && pattern_token[0] == '{' &&
            pattern_token[token_len - 1] == '}')
        {

            // Extract parameter name (between curly braces)
            if (token_len - 2 >= sizeof(request->url_params[0].key))
            {
                printf("Parameter name too long\n");
                return -1;
            }

            char param_name[MAX_PARAM_KEY_LENGTH];
            strncpy(param_name, pattern_token + 1, token_len - 2);
            param_name[token_len - 2] = '\0';

            // Validate parameter value length
            if (strlen(path_token) >= sizeof(request->url_params[0].value))
            {
                printf("Parameter value too long\n");
                return -1;
            }

            // Store parameter
            safe_strncpy(request->url_params[request->url_param_count].key,
                         param_name, sizeof(request->url_params[0].key));
            safe_strncpy(request->url_params[request->url_param_count].value,
                         path_token, sizeof(request->url_params[0].value));

            request->url_param_count++;
        }

        pattern_token = strtok_r(NULL, "/", &pattern_saveptr);
        path_token = strtok_r(NULL, "/", &path_saveptr);
    }

    return 0;
}

void http_request_cleanup(HTTP_REQUEST *request)
{
    if (!request)
        return;

    if (request->body)
    {
        free(request->body);
        request->body = NULL;
    }

    memset(request, 0, sizeof(HTTP_REQUEST));
}

void http_request_print(const HTTP_REQUEST *request)
{
    if (!request)
    {
        printf("Invalid request pointer\n");
        return;
    }

    printf("Method: %.15s\n", request->method);
    printf("Original Path: %.255s\n", request->path);
    printf("Clean Path: %.255s\n", request->clean_path);
    printf("Query String: %.255s\n", request->query_string);
    printf("Version: %.15s\n", request->version);
    printf("Content-Length: %d\n", request->content_length);
    printf("Query Parameters (%d):\n", request->query_param_count);

    for (int i = 0; i < request->query_param_count && i < MAX_QUERY_PARAMS; i++)
    {
        printf("  %.63s = %.255s\n",
               request->query_params[i].key,
               request->query_params[i].value);
    }

    printf("URL Parameters (%d):\n", request->url_param_count);
    for (int i = 0; i < request->url_param_count && i < MAX_URL_PARAMS; i++)
    {
        printf("  %.63s = %.255s\n",
               request->url_params[i].key,
               request->url_params[i].value);
    }

    if (request->body && request->content_length > 0)
    {
        printf("Body: %.*s\n", request->content_length, request->body);
    }
}