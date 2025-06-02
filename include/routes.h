#ifndef ROUTES_H
#define ROUTES_H

#include "request.h"
#include "response.h"

void route_home(const HTTP_REQUEST *request, HTTP_RESPONSE *response);
void route_about(const HTTP_REQUEST *request, HTTP_RESPONSE *response);
void route_not_found(const HTTP_REQUEST *request, HTTP_RESPONSE *response);

void route_get_users(const HTTP_REQUEST *request, HTTP_RESPONSE *response);
void route_create_user(const HTTP_REQUEST *request, HTTP_RESPONSE *response);
void route_update_user(const HTTP_REQUEST *request, HTTP_RESPONSE *response);
void route_partial_update_user(const HTTP_REQUEST *request, HTTP_RESPONSE *response);
void route_delete_user(const HTTP_REQUEST *request, HTTP_RESPONSE *response);
void route_login(const HTTP_REQUEST *request, HTTP_RESPONSE *response);

#endif // ROUTES_H