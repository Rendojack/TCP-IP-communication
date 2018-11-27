#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>

#define MAX_CLIENTS 30
#define MAX_PENDING_CONNS 3

#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1

#define TRUE 1
#define FALSE 0

#define GREETING_MSG "[Server] Welcome to BasicServer!\n"
#define MSG_LEN 1024

struct sockaddr_in address;
int addr_len;

int server_socket;
int client_socket;
int client_sockets[MAX_CLIENTS];
char* client_msg;

fd_set read_fds;

int server_port;// arg

//================================

void init_clients(void);
void init_server(void);
void accept_connections(void);
void handle_new_connections(void);
void handle_IO_operations(void);

//================================

int main(int argc, char* argv[])
{
    if(argv[1] != NULL)
    {
        server_port = strtol(argv[1], NULL, 10);

        if(server_port == 0)
        {
            printf("[ERROR] Missing arg: server PORT\n");
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        printf("[ERROR] Missing arg: server PORT\n");
        exit(EXIT_FAILURE);
    }

    init_server();
    init_clients();

    accept_connections();
}

void init_clients(void)
{
    client_msg = (char*)malloc(MSG_LEN * sizeof(char*));

    for(int i=0; i<MAX_CLIENTS; i++)
        client_sockets[i] = 0;
}

void init_server(void)
{
    // Create server socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);// TCP/IPv4
    if(server_socket == 0)
    {
        perror("[Server] Socket");
        exit(EXIT_FAILURE);
    }

    // Set server socket type
    address.sin_family = AF_INET;// IPv4
    address.sin_addr.s_addr = INADDR_ANY;// all IPs of the machine
    address.sin_port = htons(server_port);

    addr_len = sizeof(address);

    // Bind server socket to specified port
    if(bind(server_socket, (struct sockaddr*)&address, sizeof(address)) < 0)
    {
        perror("[Server] Bind");
        exit(EXIT_FAILURE);
    }

    // Try to specify max pending connections for server
    if(listen(server_socket, MAX_PENDING_CONNS) < 0)// MAX_PENDING_CONNS for kernel TCP stack
    {
        perror("[Server] Listen");
        exit(EXIT_FAILURE);
    }

    printf("[Server] Server is ready to use!\n");
    printf("[Server] Connect to any of the machine IPs, using port: %d\n", server_port);
}

void accept_connections(void)
{
    printf("[Server] Waiting for connections...\n\n");

    while(TRUE)
    {
        // Clear socket set
        FD_ZERO(&read_fds);
        // Let server socket to be watched for an activity
        FD_SET(server_socket, &read_fds);
        int highest_socket = server_socket;// for select() optimization

        // Let all child sockets to be watched by select()
        for(int i=0; i<MAX_CLIENTS; i++)
        {
            client_socket = client_sockets[i];
            if(client_socket > 0)
                FD_SET(client_socket, &read_fds);

            if(client_socket > highest_socket)
                highest_socket = client_socket;
        }

        // Wait for an activity on one of the client sockets
        // select(optimization, read fds, write fds, err fds, timeout struct(secs, microsecs))
        if(select(highest_socket + 1, &read_fds, NULL, NULL, NULL) < 0)
        {
            perror("[Server] Select");
            exit(EXIT_FAILURE);
        }

        handle_new_connections();
        handle_IO_operations();
    }
}

void handle_new_connections(void)
{
    // Server socket is triggered - new connection incoming
    if(FD_ISSET(server_socket, &read_fds))
    {
        int new_client_socket = accept(server_socket, (struct sockaddr*)&address, (socklen_t*)&addr_len);
        if(new_client_socket < 0)
        {
            perror("[Server] Accept");
            exit(EXIT_FAILURE);
        }

        printf("[Server] New connection! IP: %s:%d\n\n", inet_ntoa(address.sin_addr), ntohs(address.sin_port));

        // Send greeting message
        if(send(new_client_socket, GREETING_MSG, strlen(GREETING_MSG), 0) != strlen(GREETING_MSG))
        {
            perror("[Server] Send");
            exit(EXIT_FAILURE);
        }

        // Add new socket to socket array
        for(int i=0; i<MAX_CLIENTS; i++)
        {
            // Empty?
            if(client_sockets[i] == 0)
            {
                client_sockets[i] = new_client_socket;
                break;
            }
        }
    }
}

void handle_IO_operations(void)
{
    for(int i=0; i<MAX_CLIENTS; i++)
    {
        client_socket = client_sockets[i];

        // Client socket is triggered, some I/O operation
        if(FD_ISSET(client_socket, &read_fds))
        {
            memset(client_msg, 0, MSG_LEN * sizeof(char*));
            if(recv(client_socket, client_msg, MSG_LEN * sizeof(char*), 0) == 0)
            {
                getpeername(client_socket, (struct sockaddr*)&address, (socklen_t*)&addr_len);
                printf("[Server] Client disconnected, IP: %s:%d\n\n", inet_ntoa(address.sin_addr), ntohs(address.sin_port));

                shutdown(client_socket, SHUT_RDWR);
                close(client_socket);
                client_sockets[i] = 0;
            }
            else
            {
                getpeername(client_socket, (struct sockaddr*)&address, (socklen_t*)&addr_len);
                printf("[Server] Message from client IP: %s:%d\n", inet_ntoa(address.sin_addr), ntohs(address.sin_port));
                printf("[Client] %s\n", client_msg);

                char* message_back = "[Server] Message delivered\n";
                send(client_socket, message_back, strlen(message_back), 0);
            }
        }
    }
}
