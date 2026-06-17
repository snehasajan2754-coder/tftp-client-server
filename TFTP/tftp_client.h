#ifndef TFTP_CLIENT_H
#define TFTP_CLIENT_H

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

typedef struct {
    int sockfd;
    struct sockaddr_in server_addr;
    socklen_t server_len;
    char server_ip[INET_ADDRSTRLEN];
    char mode[16];
    int connected;   // flag
} tftp_client_t;

// Function prototypes
void connect_to_server(tftp_client_t *client, char *ip, int port);
void put_file(tftp_client_t *client, char *filename);
void get_file(tftp_client_t *client, char *filename);
void disconnect(tftp_client_t *client);


void send_request(int sockfd, struct sockaddr_in server_addr, char *filename, int opcode);
void receive_request(int sockfd, struct sockaddr_in server_addr, char *filename, int opcode);
#endif