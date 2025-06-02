#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "../include/config.h"
#include "../include/server.h"
#include "../include/handler.h"
#include "../include/database.h"

static TCP_SERVER server;
Database app_db;

void signal_handler(int sig)
{
    (void)sig;

    printf("\nShutting down server...\n");
    server_close(&server);
    db_close(&app_db);
    exit(0);
}

int main(int argc, char *argv[])
{
    int port = DEFAULT_PORT;

    // Parse command line arguments
    if (argc > 1)
    {
        port = atoi(argv[1]);
        if (port <= 0 || port > 65535)
        {
            printf("Invalid port number. Using default port %d\n", DEFAULT_PORT);
            port = DEFAULT_PORT;
        }
    }

    // Set up signal handler for graceful shutdown
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    printf("Starting HTTP server on port %d...\n", port);

    // Create and configure server
    if (server_create(&server, port) < 0)
    {
        exit(1);
    }

    if (server_bind(&server) < 0)
    {
        server_close(&server);
        exit(1);
    }

    if (server_listen(&server, MAX_CONNECTIONS) < 0)
    {
        server_close(&server);
        exit(1);
    }

    // Initialize database
    if (db_init(&app_db, "httpserver.db") < 0)
    {
        fprintf(stderr, "Failed to initialize database\n");
        exit(1);
    }

    if (db_create_tables(&app_db) < 0)
    {
        fprintf(stderr, "Failed to create tables\n");
        db_close(&app_db);
        exit(1);
    }

    printf("Server listening on http://localhost:%d\n", port);
    printf("Press Ctrl+C to stop the server\n\n");

    // Main server loop
    while (1)
    {
        struct sockaddr_in client_addr;
        int client_socket = server_accept(&server, &client_addr);

        if (client_socket < 0)
        {
            continue;
        }

        printf("New connection from %s:%d\n",
               inet_ntoa(client_addr.sin_addr),
               ntohs(client_addr.sin_port));

        // Handle the request
        handle_http_request(client_socket);

        // Close client connection
        close(client_socket);
    }

    server_close(&server);
    db_close(&app_db);
    return 0;
}