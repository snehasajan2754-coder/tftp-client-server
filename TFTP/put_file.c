#include "tftp_client.h"
#include "tftp.h"
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>   // for read, close
#include <fcntl.h>    // for open, O_RDONLY

void put_file(tftp_client_t *client, char *file_name)
{
    int fd = open(file_name, O_RDONLY);     // Open file for reading
    if (fd < 0)
    {
        perror("File not found");
        return;
    }

    tftp_packet pkt, reply;
    memset(&pkt, 0, sizeof(pkt));

    // Step 1: Send WRQ (Write Request)
    pkt.opcode = htons(WRQ);
    strcpy(pkt.body.request.filename, file_name);
    strcpy(pkt.body.request.mode, client->mode);

    if (sendto(client->sockfd, &pkt, sizeof(pkt), 0,(struct sockaddr *)&client->server_addr, client->server_len) < 0)
    {
        perror("WRQ send error");
        close(fd);
        return;
    }

    // Step 2: Wait for ACK to WRQ
    if (recvfrom(client->sockfd, &reply, sizeof(reply), 0,(struct sockaddr *)&client->server_addr, &client->server_len) < 0)
    {
        perror("WRQ ACK receive error");
        close(fd);
        return;
    }

    uint16_t block = 1;
    ssize_t read_bytes;

    // Step 3: Send file data in 512‑byte blocks
    while (1)
    {
        memset(&pkt, 0, sizeof(pkt));
        pkt.opcode = htons(DATA);

        read_bytes = read(fd, pkt.body.data_packet.data, 512);
        if (read_bytes < 0)
        {
            perror("File read error");
            break;
        }

        pkt.body.data_packet.block_number = htons(block);
        pkt.body.data_packet.data_length = htons(read_bytes);

        // Send DATA packet (header + actual data length)
        ssize_t sent_bytes = sendto(client->sockfd, &pkt,sizeof(pkt.opcode) + sizeof(pkt.body.data_packet.block_number) +sizeof(pkt.body.data_packet.data_length) + read_bytes,0, (struct sockaddr *)&client->server_addr, client->server_len);

        if (sent_bytes < 0)
        {
            perror("Data send error");
            break;
        }

        // Wait for ACK
        ssize_t ack_bytes = recvfrom(client->sockfd, &reply, sizeof(reply), 0,(struct sockaddr *)&client->server_addr, &client->server_len);

        if (ack_bytes < 0)
        {
            perror("ACK receive error, resending block");
            continue; // retry same block
        }

        if (ntohs(reply.opcode) != ACK || ntohs(reply.body.ack_packet.block_number) != block)
        {
            printf("Invalid ACK received\n");
            break;
        }

        block++; // Next block

        if (read_bytes < 512) // Last packet
        {
            printf("File transfer completed\n");
            break;
        }
    }

    close(fd);
}
