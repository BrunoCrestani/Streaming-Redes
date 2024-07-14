#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/socket.h>
#include <net/ethernet.h> 
#include <linux/if_packet.h> 
#include <sys/types.h>
#include <net/if.h> 
#include <arpa/inet.h> 
#include "../message/message.h"

#define BUFFER_SIZE 1024

int main() {
    unsigned int rsocket = rawSocketCreator("lo");

    struct sockaddr_ll client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    uint8_t buffer[BUFFER_SIZE];

    while (1) {
        ssize_t receivedBytes = recvfrom(rsocket, buffer, BUFFER_SIZE, 0, (struct sockaddr*)&client_addr, &client_addr_len);
        if (receivedBytes == -1) {
            perror("Erro ao receber mensagem");
            exit(-1);
        }

        // Assuming buffer contains the received message
        Message receivedMessage;
        memcpy(&receivedMessage, buffer, sizeof(Message));

        // write received message to binary file
        FILE *file = fopen("received_message.bin", "wb");
        fwrite(&receivedMessage, sizeof(Message), 1, file);
        fclose(file);

        printf("Pacote recebido:\n");
        printf("Start Marker: 0x%X\n", receivedMessage.marker);
        printf("Message Size: %u bytes\n", receivedMessage.size);
        printf("Sequence: %u\n", receivedMessage.sequence);
        printf("Message Type: %u\n", receivedMessage.type);
        printf("Data: %s\n", receivedMessage.data);
        printf("Error Detection: 0x%X\n", receivedMessage.error);
        printf("\n");
    }

    close(rsocket);
    return 0;
}
