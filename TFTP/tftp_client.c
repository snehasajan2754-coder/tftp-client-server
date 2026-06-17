#include "tftp.h"
#include "tftp_client.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

void disconnect(tftp_client_t *client) 
{
    if (client->connected)
    {
        close(client->sockfd);      // Close the socket file descriptor
        client->connected = 0;      // Update status
        printf("Disconnected from server\n");
    }
   
}
int main()
{
    tftp_client_t client;
    memset(&client, 0, sizeof(client));

    int choice;
    char ip[INET_ADDRSTRLEN];
    char filename[256];

    while (1)
    {
        // Print menu each loop
        printf("\n========== TFTP CLIENT ==========\n");
        printf("1. Connect\n2. Put (Upload)\n3. Get (Download)\n4. Mode\n5. Exit\nChoose option : ");
        scanf("%d", &choice);

        switch (choice)
        {
            case 1:
                printf("Enter Server IP : ");
                scanf("%s", ip);
                connect_to_server(&client, ip, PORT);
                break;

            case 2:
                printf("Enter filename to upload : ");
                scanf("%s", filename);
                put_file(&client, filename);
                break;

            case 3:
                printf("Enter filename to download : ");
                scanf("%s", filename);
                get_file(&client, filename);
                break;

            case 4:
                int mode;
                printf("Select mode\n1. Octet\n2. Netascii\n");
                scanf("%d",&mode);
                if (mode == 1)
                {
                    strcpy(client.mode, "octet");
                }
                else if (mode == 2)
                {
                    strcpy(client.mode, "netascii");
                }
                else
                {
                    printf("Invalid mode\n");
                }
                printf("Mode set to %s\n", client.mode);
                break;

            case 5:
                disconnect(&client);
                printf("Exiting client\n");
                return 0;

            default:
                printf("Invalid option\n");
        }
    }
}