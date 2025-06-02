#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "../include/config.h"
#include "../include/server.h"

int server_create(TCP_SERVER *server, int port)
{
    server->port = port;

    // Create socket
    server->socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server->socket_fd < 0)
    {
        perror("Socket creation failed");
        return -1;
    }

    // Set socket options to reuse address
    int opt = 1;
    if (setsockopt(server->socket_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        perror("setsockopt failed");
        close(server->socket_fd);
        return -1;
    }

    // Configure server address
    memset(&server->address, 0, sizeof(server->address));
    server->address.sin_family = AF_INET;
    server->address.sin_addr.s_addr = INADDR_ANY;
    server->address.sin_port = htons(port);

    return 0;
}

int server_bind(TCP_SERVER *server)
{
    if (bind(server->socket_fd, (struct sockaddr *)&server->address, sizeof(server->address)) < 0)
    {
        perror("Bind failed");
        return -1;
    }
    return 0;
}

int server_listen(TCP_SERVER *server, int backlog)
{
    if (listen(server->socket_fd, backlog) < 0)
    {
        perror("Listen failed");
        return -1;
    }
    return 0;
}

int server_accept(TCP_SERVER *server, struct sockaddr_in *client_addr)
{
    socklen_t client_len = sizeof(*client_addr);
    int client_socket = accept(server->socket_fd, (struct sockaddr *)client_addr, &client_len);
    if (client_socket < 0)
    {
        perror("Accept failed");
        return -1;
    }
    return client_socket;
}

void server_close(TCP_SERVER *server)
{
    if (server->socket_fd >= 0)
    {
        close(server->socket_fd);
        server->socket_fd = -1;
    }
}