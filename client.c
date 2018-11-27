#define _GNU_SOURCE

#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>

#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1

#define TRUE 1
#define FALSE 0

#define MSG_LEN 1024

struct sockaddr_in address;
int client_socket;

int server_port;// arg
int client_port;// arg
char* server_ip;// arg

char* client_msg;
char* server_msg;

//================================

void print_server_msg(void);
void init_client(void);
void connect_to_server(void);
void loop_client_input(void);
char* get_line(void);

//================================

int main(int argc, char* argv[])
{
    if(argv[1] != NULL && argv[2] != NULL && argv[3] != NULL)
    {
        server_ip = argv[1];
        server_port = strtol(argv[2], NULL, 10);// (str, endptr, base)
        client_port = strtol(argv[3], NULL, 10);

        if(server_port == 0 || client_port == 0)
        {
            printf("[ERROR] Missing first arg: server IP and/or second arg: server PORT and/or third arg: client port\n");
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        printf("[ERROR] Missing first arg: server IP and/or second arg: server PORT and/or third arg: client port\n");
        exit(EXIT_FAILURE);
    }

    init_client();
    connect_to_server();
    loop_client_input();
}

void print_server_msg(void)
{
    if(recv(client_socket, server_msg, MSG_LEN * sizeof(char*), 0) <= 0)
    {
        printf("[Client] Server disconnected\n");
        exit(EXIT_FAILURE);
    }
    else
    {
        printf(server_msg);
        memset(server_msg, 0, MSG_LEN * sizeof(*server_msg));
    }
}

void init_client(void)
{
    server_msg = (char*)malloc(MSG_LEN * sizeof(char*));
    client_msg = (char*)malloc(MSG_LEN * sizeof(char*));

    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if(client_socket < 0)
    {
        perror("[Client] Socket");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;// IPv4
    address.sin_addr.s_addr = INADDR_ANY;// all IPs of the machine
    address.sin_port = htons(client_port);// host to network byte-order

    if(bind(client_socket, (struct sockaddr*)&address, sizeof(struct sockaddr_in)) != 0)
    {
        perror("[Client] Bind");
        exit(EXIT_FAILURE);
    }
}

void connect_to_server(void)
{
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;// IPv4
    address.sin_port = htons(server_port);// host to network byte-order

    if(inet_aton(server_ip, (struct in_addr*)&address.sin_addr.s_addr) == 0)
    {
        perror(server_ip);
        exit(EXIT_FAILURE);
    }

    if(connect(client_socket, (struct sockaddr*)&address, sizeof(address)) != 0)
    {
        perror("[Client] Connection failed");
        exit(EXIT_FAILURE);
    }
    else
    {
        printf("[Client] Connection established!\n");
        print_server_msg();
    }
}

void loop_client_input(void)
{
    while(TRUE)
    {
        printf("[Client] Message to send: ");

        memset(client_msg, 0, MSG_LEN * sizeof(char*));

        size_t msg_size = MSG_LEN;
        getline(&client_msg, &msg_size, stdin);

        if(send(client_socket, client_msg, strlen(client_msg), 0) != strlen(client_msg))
        {
            perror("[Client] Send");
            exit(EXIT_FAILURE);
        }
        print_server_msg();
    }
}
