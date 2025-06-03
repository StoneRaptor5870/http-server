#include <sys/stat.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "../include/file.h"

const char *get_mime_type(const char *filename)
{
    const char *ext = strrchr(filename, '.');
    if (!ext)
    {
        printf("No extension found for file: %s, using default MIME type\n", filename);
        return "application/octet-stream"; // Default for unknown types
    }

    // Convert extension to lowercase for comparison
    char ext_lower[16];
    size_t i = 0; // Changed from int to size_t
    while (ext[i] && i < sizeof(ext_lower) - 1)
    {
        ext_lower[i] = (ext[i] >= 'A' && ext[i] <= 'Z') ? ext[i] + 32 : ext[i];
        i++;
    }
    ext_lower[i] = '\0';

    // Find matching MIME type
    for (int j = 0; mime_types[j].extension != NULL; j++)
    {
        if (strcmp(ext_lower, mime_types[j].extension) == 0)
        {
            return mime_types[j].mime_type;
        }
    }

    return "application/octet-stream";
}

// Check if path is safe (no directory traversal)
int is_safe_path(const char *path)
{
    // Check for directory traversal attempts
    if (strstr(path, "..") != NULL)
    {
        return 0;
    }

    // Check for absolute paths
    if (path[0] == '/')
    {
        return 0;
    }

    // Additional security checks can be added here
    return 1;
}

int construct_file_path(const char *requested_path, char *full_path, size_t max_len)
{
    // Remove leading slash if present
    const char *clean_path = (requested_path[0] == '/') ? requested_path + 1 : requested_path;

    // Handle empty path (serve index.html)
    if (strlen(clean_path) == 0 || strcmp(clean_path, "/") == 0)
    {
        clean_path = "index.html";
    }
    else if (strlen(clean_path) == 5 || strcmp(clean_path, "/about") == 0)
    {
        clean_path = "about.html";
    }
    else if (strlen(clean_path) == 13 || strcmp(clean_path, "/css/style.css") == 0)
    {
        clean_path = "css/style.css";
    }
    else if (strlen(clean_path) == 9 || strcmp(clean_path, "/js/app.js") == 0)
    {
        clean_path = "js/app.js";
    }

    // Check path safety
    if (!is_safe_path(clean_path))
    {
        return -1;
    }

    // Construct full path
    int result = snprintf(full_path, max_len, "%s/%s", STATIC_FILES_DIR, clean_path);
    if (result >= (int)max_len)
    {
        return -1; // Path too long
    }

    return 0;
}

long read_file_contents(const char *filepath, char **buffer)
{
    FILE *file = fopen(filepath, "rb");
    if (!file)
    {
        return -1;
    }

    // Get file size
    if (fseek(file, 0, SEEK_END) != 0)
    {
        fclose(file);
        return -1;
    }

    long file_size = ftell(file);
    if (file_size < 0 || file_size > MAX_FILE_SIZE)
    {
        fclose(file);
        return -1;
    }

    if (fseek(file, 0, SEEK_SET) != 0)
    {
        fclose(file);
        return -1;
    }

    // Allocate buffer
    *buffer = malloc(file_size + 1);
    if (!*buffer)
    {
        fclose(file);
        return -1;
    }

    // Read file
    size_t bytes_read = fread(*buffer, 1, file_size, file);
    fclose(file);

    if ((long)bytes_read != file_size)
    {
        free(*buffer);
        *buffer = NULL;
        return -1;
    }

    (*buffer)[file_size] = '\0'; // Null terminate for text files
    return file_size;
}

void serve_static_file(const HTTP_REQUEST *request, HTTP_RESPONSE *response)
{
    char full_path[MAX_PATH_LENGTH];
    char *file_contents = NULL;
    long file_size;

    printf("Serving static file for path: %s\n", request->path);

    // Construct file path
    if (construct_file_path(request->path, full_path, sizeof(full_path)) < 0)
    {
        printf("Invalid or unsafe path: %s\n", request->path);
        serve_404_error(response);
        return;
    }

    printf("Full file path: %s\n", full_path);

    // Check if file exists and is readable
    struct stat file_stat;
    if (stat(full_path, &file_stat) != 0)
    {
        printf("File not found: %s\n", full_path);
        serve_404_error(response);
        return;
    }

    // Check if it's a regular file
    if (!S_ISREG(file_stat.st_mode))
    {
        printf("Not a regular file: %s\n", full_path);
        serve_404_error(response);
        return;
    }

    // Read file contents
    file_size = read_file_contents(full_path, &file_contents);
    if (file_size < 0 || !file_contents)
    {
        printf("Failed to read file: %s\n", full_path);
        serve_500_error(response);
        return;
    }

    // Get MIME type
    const char *mime_type = get_mime_type(full_path);

    // Set response
    http_response_set_status(response, HTTP_200_OK);
    http_response_set_content_type(response, mime_type);
    http_response_set_body_with_length(response, file_contents, file_size);

    printf("Successfully served file: %s (size: %ld bytes, type: %s)\n",
           full_path, file_size, mime_type);
}

void serve_404_error(HTTP_RESPONSE *response)
{
    const char *error_html =
        "<!DOCTYPE html>\n"
        "<html>\n"
        "<head>\n"
        "    <title>404 - File Not Found</title>\n"
        "    <style>\n"
        "        body { font-family: Arial, sans-serif; text-align: center; margin-top: 100px; }\n"
        "        .error { color: #e74c3c; }\n"
        "        .back-link { margin-top: 20px; }\n"
        "        .back-link a { color: #3498db; text-decoration: none; }\n"
        "    </style>\n"
        "</head>\n"
        "<body>\n"
        "    <div class=\"error\">\n"
        "        <h1>404 - File Not Found</h1>\n"
        "        <p>The requested file could not be found on this server.</p>\n"
        "    </div>\n"
        "    <div class=\"back-link\">\n"
        "        <a href=\"/\">← Back to Home</a>\n"
        "    </div>\n"
        "</body>\n"
        "</html>\n";

    http_response_set_status(response, HTTP_404_NOT_FOUND);
    http_response_set_content_type(response, "text/html");
    http_response_set_body(response, error_html);
}

void serve_500_error(HTTP_RESPONSE *response)
{
    const char *error_html =
        "<!DOCTYPE html>\n"
        "<html>\n"
        "<head>\n"
        "    <title>500 - Internal Server Error</title>\n"
        "    <style>\n"
        "        body { font-family: Arial, sans-serif; text-align: center; margin-top: 100px; }\n"
        "        .error { color: #e74c3c; }\n"
        "        .back-link { margin-top: 20px; }\n"
        "        .back-link a { color: #3498db; text-decoration: none; }\n"
        "    </style>\n"
        "</head>\n"
        "<body>\n"
        "    <div class=\"error\">\n"
        "        <h1>500 - Internal Server Error</h1>\n"
        "        <p>An error occurred while processing your request.</p>\n"
        "    </div>\n"
        "    <div class=\"back-link\">\n"
        "        <a href=\"/\">← Back to Home</a>\n"
        "    </div>\n"
        "</body>\n"
        "</html>\n";

    http_response_set_status(response, HTTP_500_INTERNAL_ERROR);
    http_response_set_content_type(response, "text/html");
    http_response_set_body(response, error_html);
}