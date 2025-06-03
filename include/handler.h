#ifndef HANDLER_H
#define HANDLER_H

#include "request.h"
#include "response.h"

// Route handler function pointer type
typedef void (*ROUTE_HANDLER)(const HTTP_REQUEST *request, HTTP_RESPONSE *response);

// HTTP handler functions
void handle_http_request(int client_socket);

// Method-specific handlers
void handle_get_request(const HTTP_REQUEST *request, HTTP_RESPONSE *response);
void handle_post_request(const HTTP_REQUEST *request, HTTP_RESPONSE *response);
void handle_put_request(const HTTP_REQUEST *request, HTTP_RESPONSE *response);
void handle_patch_request(const HTTP_REQUEST *request, HTTP_RESPONSE *response);
void handle_delete_request(const HTTP_REQUEST *request, HTTP_RESPONSE *response);
void route_get_js(const HTTP_REQUEST *request, HTTP_RESPONSE *response);
void route_get_css(const HTTP_REQUEST *request, HTTP_RESPONSE *response);

void route_method_not_allowed(const HTTP_REQUEST *request, HTTP_RESPONSE *response);

#endif // HANDLER_H