#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "../include/config.h"
#include "../include/server.h"
#include "../include/handler.h"
#include "../include/database.h"
#include "../include/threadpool.h"

static TCP_SERVER server;
Database app_db;
ThreadPool *thread_pool = NULL;

// Structure to hold client request data
typedef struct
{
    int client_socket;
    struct sockaddr_in client_addr;
} ClientRequest;

void signal_handler(int sig)
{
    (void)sig;

    printf("\nShutting down server...\n");

    // Destroy thread pool first
    if (thread_pool)
    {
        printf("Destroying thread pool...\n");
        threadpool_destroy(thread_pool);
        thread_pool = NULL;
    }

    // Close server socket
    server_close(&server);

    // Close database
    db_close(&app_db);

    printf("Server shutdown complete\n");
    exit(0);
}

// Thread function to handle client request
void handle_client_request(void *arg)
{
    ClientRequest *client_req = (ClientRequest *)arg;
    int client_socket = client_req->client_socket;

    printf("[Thread %lu] Handling client request from %s:%d\n",
           pthread_self(),
           inet_ntoa(client_req->client_addr.sin_addr),
           ntohs(client_req->client_addr.sin_port));

    handle_http_request(client_socket);

    // Close client socket
    close(client_socket);

    // Free the client request structure
    free(client_req);

    printf("[Thread %lu] Request handling completed\n", pthread_self());
}

int main(int argc, char *argv[])
{
    int port = DEFAULT_PORT;
    int thread_count = DEFAULT_THREAD_COUNT;
    int queue_size = MAX_QUEUE_SIZE;

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

    if (argc > 2)
    {
        thread_count = atoi(argv[2]);
        if (thread_count <= 0 || thread_count > 5)
        {
            printf("Invalid thread count. Using default %d threads\n", DEFAULT_THREAD_COUNT);
            thread_count = DEFAULT_THREAD_COUNT;
        }
    }

    if (argc > 3)
    {
        queue_size = atoi(argv[3]);
        if (queue_size <= 0 || queue_size > 100)
        {
            printf("Invalid queue size. Using default %d\n", MAX_QUEUE_SIZE);
            queue_size = MAX_QUEUE_SIZE;
        }
    }

    // Set up signal handler for graceful shutdown
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    printf("Starting HTTP server on port %d with %d threads...\n", port, thread_count);

    // Create thread pool
    thread_pool = threadpool_create(thread_count, queue_size);
    if (!thread_pool)
    {
        fprintf(stderr, "Failed to create thread pool\n");
        exit(1);
    }

    // Create and configure server
    if (server_create(&server, port) < 0)
    {
        threadpool_destroy(thread_pool);
        exit(1);
    }

    if (server_bind(&server) < 0)
    {
        server_close(&server);
        threadpool_destroy(thread_pool);
        exit(1);
    }

    if (server_listen(&server, MAX_CONNECTIONS) < 0)
    {
        server_close(&server);
        threadpool_destroy(thread_pool);
        exit(1);
    }

    // Initialize database
    if (db_init(&app_db, "httpserver.db") < 0)
    {
        fprintf(stderr, "Failed to initialize database\n");
        server_close(&server);
        threadpool_destroy(thread_pool);
        exit(1);
    }

    if (db_create_tables(&app_db) < 0)
    {
        fprintf(stderr, "Failed to create tables\n");
        db_close(&app_db);
        server_close(&server);
        threadpool_destroy(thread_pool);
        exit(1);
    }

    printf("Server listening on http://localhost:%d\n", port);
    printf("Thread pool: %d worker threads with queue size %d\n", thread_count, queue_size);
    printf("Press Ctrl+C to stop the server\n\n");

    // Main server loop
    while (1)
    {
        struct sockaddr_in client_addr;
        int client_socket = server_accept(&server, &client_addr);

        if (client_socket < 0)
        {
            // Accept failed, but continue running
            continue;
        }

        printf("New connection from %s:%d\n",
               inet_ntoa(client_addr.sin_addr),
               ntohs(client_addr.sin_port));

        // Create client request structure
        ClientRequest *client_req = malloc(sizeof(ClientRequest));
        if (!client_req)
        {
            fprintf(stderr, "Failed to allocate memory for client request\n");
            close(client_socket);
            continue;
        }

        client_req->client_socket = client_socket;
        client_req->client_addr = client_addr;

        // Add task to the thread pool
        if (threadpool_add_task(thread_pool, handle_client_request, client_req) < 0)
        {
            fprintf(stderr, "Failed to add task to thread pool\n");
            close(client_socket);
            free(client_req);
            continue;
        }
    }

    threadpool_destroy(thread_pool);
    server_close(&server);
    db_close(&app_db);
    return 0;
}