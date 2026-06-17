#include "tftp_client.h"
#include "tftp.h"

// This function is to initialize socket with given server IP, no packets sent to server in this function
void connect_to_server(tftp_client_t *client, char *ip, int port)
{
    if (client->connected)
    {
        disconnect(client);   // Disconnect if already connected
    }

    // Create UDP socket
    client->sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (client->sockfd < 0)
    {
        perror("Socket creation failed");
        return;
    }

    // Set socket timeout option (5 seconds)
    struct timeval tv;
    tv.tv_sec = 5;
    tv.tv_usec = 0;
    if (setsockopt(client->sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0)
    {
        perror("Error setting socket timeout");
    }

    // Set up server address
    memset(&client->server_addr, 0, sizeof(client->server_addr));
    client->server_addr.sin_family = AF_INET;
    client->server_addr.sin_port = htons(port);

    // Validate IP address string
    if (inet_pton(AF_INET, ip, &client->server_addr.sin_addr) <= 0)
    {
        printf("Invalid IP address: %s\n", ip);
        close(client->sockfd);
        return;
    }

    client->server_len = sizeof(client->server_addr);

    strncpy(client->server_ip, ip, INET_ADDRSTRLEN - 1);
    client->server_ip[INET_ADDRSTRLEN - 1] = '\0';

    client->connected = 1;
    printf("Connected to %s:%d\n", ip, port);
}
