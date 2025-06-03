#ifndef FILE_H
#define FILE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"
#include "routes.h"
#include "utils.h"

// MIME type structure
typedef struct {
    const char *extension;
    const char *mime_type;
} mime_type;

// Common MIME types
static const mime_type mime_types[] = {
    {".html", "text/html"},
    {".htm", "text/html"},
    {".css", "text/css"},
    {".js", "application/javascript"},
    {".json", "application/json"},
    {".png", "image/png"},
    {".jpg", "image/jpeg"},
    {".jpeg", "image/jpeg"},
    {".gif", "image/gif"},
    {".ico", "image/x-icon"},
    {".svg", "image/svg+xml"},
    {".txt", "text/plain"},
    {".pdf", "application/pdf"},
    {".xml", "application/xml"},
    {".zip", "application/zip"},
    {".mp4", "video/mp4"},
    {".mp3", "audio/mpeg"},
    {".wav", "audio/wav"},
    {".woff", "font/woff"},
    {".woff2", "font/woff2"},
    {".ttf", "font/ttf"},
    {".otf", "font/otf"},
    {NULL, "application/octet-stream"} // Sentinel
};

void serve_static_file(const HTTP_REQUEST *request, HTTP_RESPONSE *response);
void serve_404_error(HTTP_RESPONSE *response);
void serve_500_error(HTTP_RESPONSE *response);
const char* get_mime_type(const char* filename);

#endif // FILE_H