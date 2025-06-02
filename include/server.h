#ifndef SERVER_H
#define SERVER_H

#include <sys/socket.h>
#include <netinet/in.h>

typedef struct
{
    int socket_fd;
    int port;
    struct sockaddr_in address;
} TCP_SERVER;

// TCP server functions
int server_create(TCP_SERVER *server, int port);
int server_bind(TCP_SERVER *server);
int server_listen(TCP_SERVER *server, int backlog);
int server_accept(TCP_SERVER *server, struct sockaddr_in *client_addr);
void server_close(TCP_SERVER *server);

#endif // SERVER_H