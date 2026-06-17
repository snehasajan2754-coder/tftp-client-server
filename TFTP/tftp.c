/* Common file for server & client */

#include "tftp.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>   // for read, write, close
#include <fcntl.h>    // for open, O_CREAT, O_WRONLY, O_TRUNC

void send_file(int sockfd, struct sockaddr_in client_addr, socklen_t client_len, char *filename) 
{
    int fd = open(filename, O_RDONLY);
    if (fd < 0) {
        perror("File open error");
        return;
    }

    tftp_packet pkt, ack;
    uint16_t block = 1;
    ssize_t read_bytes;

    while (1) {
        memset(&pkt, 0, sizeof(pkt));
        pkt.opcode = htons(DATA);
        pkt.body.data_packet.block_number = htons(block);

        read_bytes = read(fd, pkt.body.data_packet.data, 512);
        if (read_bytes < 0) 
        {
            perror("File read error");
            break;
        }

        pkt.body.data_packet.data_length = htons(read_bytes);

        // Send DATA packet
        sendto(sockfd, &pkt,sizeof(pkt.opcode) + sizeof(pkt.body.data_packet.block_number) +sizeof(pkt.body.data_packet.data_length) + read_bytes, 0, (struct sockaddr *)&client_addr, client_len);

        // Wait for ACK
        int bytes = recvfrom(sockfd, &ack, sizeof(ack), 0,(struct sockaddr *)&client_addr, &client_len);
        if (bytes < 0) 
        {
            perror("ACK receive error");
            continue; // retry same block
        }

        if (ntohs(ack.opcode) != ACK || ntohs(ack.body.ack_packet.block_number) != block) 
        {
            printf("Invalid ACK received\n");
            break;
        }

        block++;

        if (read_bytes < 512) { // Last packet
            printf("File sent successfully\n");
            break;
        }
    }

    close(fd);
}


void receive_file(int sockfd, struct sockaddr_in client_addr, socklen_t client_len, char *filename) 
{
    int fd = open(filename, O_CREAT | O_WRONLY | O_TRUNC, 0666);
    if (fd < 0) 
    {
        perror("File open error");
        return;
    }

    tftp_packet pkt, reply;
    uint16_t expected_block = 1;

    while (1)
    {
        // Receive DATA packet
        int bytes = recvfrom(sockfd, &reply, sizeof(reply), 0,(struct sockaddr *)&client_addr, &client_len);
        if (bytes < 0) {
            perror("Receive error");
            break;
        }

        if (ntohs(reply.opcode) != DATA || ntohs(reply.body.data_packet.block_number) != expected_block) 
        {
            printf("Unexpected packet received\n");
            break;
        }

        uint16_t len = ntohs(reply.body.data_packet.data_length);

        // Write data to file
        write(fd, reply.body.data_packet.data, len);

        // Send ACK
        memset(&pkt, 0, sizeof(pkt));
        pkt.opcode = htons(ACK);
        pkt.body.ack_packet.block_number = htons(expected_block);

        sendto(sockfd, &pkt,sizeof(pkt.opcode) + sizeof(pkt.body.ack_packet.block_number),0, (struct sockaddr *)&client_addr, client_len);

        if (len < 512) 
        {
            printf("File received successfully\n");
            break;
        }

        expected_block++;
    }

    close(fd);
}
