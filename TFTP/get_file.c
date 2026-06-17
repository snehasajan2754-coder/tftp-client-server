#include "tftp_client.h"
#include "tftp.h"
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>

void get_file(tftp_client_t *client, char *file_name)
{
    tftp_packet pkt, reply;

    // Step 1: Send RRQ (Read Request)
    memset(&pkt, 0, sizeof(pkt));
    pkt.opcode = htons(RRQ);
    strcpy(pkt.body.request.filename, file_name);
    strcpy(pkt.body.request.mode, client->mode);

    if (sendto(client->sockfd, &pkt, sizeof(pkt), 0, (struct sockaddr *)&client->server_addr, client->server_len) < 0)
    {
        perror("RRQ send error");
        return;
    }

    // Step 2: Create local file
    int fd = open(file_name, O_CREAT | O_WRONLY | O_TRUNC, 0666);
    if (fd < 0)
    {
        perror("File open error");
        return;
    }

    uint16_t expected_block = 1;

    while (1)
    {
        // Step 3: Receive DATA packet
        int bytes = recvfrom(client->sockfd, &reply, sizeof(reply), 0, (struct sockaddr *)&client->server_addr, &client->server_len);
        if (bytes < 0)
        {
            perror("Receive error");
            break;
        }

        if (ntohs(reply.opcode) != DATA || ntohs(reply.body.data_packet.block_number) != expected_block)
        {
            printf("Unexpected packet received\n");
            break;
        }

        uint16_t len = ntohs(reply.body.data_packet.data_length);

        // Step 4: Write data to file
        if (write(fd, reply.body.data_packet.data, len) < 0)
        {
            perror("File write error");
            break;
        }

        // Step 5: Send ACK
        memset(&pkt, 0, sizeof(pkt));
        pkt.opcode = htons(ACK);
        pkt.body.ack_packet.block_number = htons(expected_block);

        if (sendto(client->sockfd, &pkt, sizeof(pkt.opcode) + sizeof(pkt.body.ack_packet.block_number), 0, (struct sockaddr *)&client->server_addr, client->server_len) < 0)
        {
            perror("ACK send error");
            break;
        }

        if (len < 512)
        {
            printf("File received successfully\n");
            break;
        }

        expected_block++;
    }

    close(fd);
}
