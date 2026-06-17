#include "tftp_client.h"
#include "tftp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <fcntl.h>

void handle_client(int sockfd, struct sockaddr_in client_addr, socklen_t client_len, tftp_packet *packet)
{
    uint16_t opcode = ntohs(packet->opcode);

    if (opcode == RRQ)   // Read request (get)
    {
        printf("RRQ received for file: %s\n", packet->body.request.filename);
        send_file(sockfd, client_addr, client_len, packet->body.request.filename);
    }
    else if (opcode == WRQ)   // Write request (put)
    {
        printf("WRQ received for file: %s\n", packet->body.request.filename);
        // Send ACK for WRQ (block = 0)
        tftp_packet ack;
        memset(&ack, 0, sizeof(ack));
        ack.opcode = htons(ACK);
        ack.body.ack_packet.block_number = htons(0);

        sendto(sockfd, &ack,sizeof(ack.opcode) + sizeof(ack.body.ack_packet.block_number),0, (struct sockaddr *)&client_addr, client_len);

        // Now receive the file data
        receive_file(sockfd, client_addr, client_len, packet->body.request.filename);
    }
    else
    {
        printf("Unknown request opcode: %d\n", opcode);
    }
}

int main()
{
    int sockfd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    tftp_packet packet;

    // Create UDP socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
    {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Set socket timeout option (5 seconds)
    struct timeval tv;
    tv.tv_sec = 5;
    tv.tv_usec = 0;
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0)
    {
        perror("Error setting socket timeout");
    }

    // Set up server address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    // Bind the socket
    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Bind failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    printf("TFTP Server listening on port %d...\n", PORT);

    // Main loop to handle incoming requests
    while (1)
    {
        struct sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);
        tftp_packet pkt;
        int n = recvfrom(sockfd, &packet, BUFFER_SIZE, 0,(struct sockaddr *)&client_addr, &client_len);
        if (n < 0)
        {
            // perror("Receive failed or timeout occurred");
            // continue;
            if (errno != EAGAIN && errno != EWOULDBLOCK) 
            {
                perror("Actual socket error"); 
            }
            // If it WAS a timeout, this 'continue' quietly goes back to the top
            continue; 
        }

        handle_client(sockfd, client_addr, client_len, &packet);
    }

    close(sockfd);
    return 0;
}