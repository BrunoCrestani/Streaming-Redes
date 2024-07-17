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
#include "../raw_sockets/sockets.h"
#include "../message/message.h"

void sendFile(int rsocket, char* filename) {
    FILE* file = fopen(filename, "r");

    if (file == NULL) {
        perror("Erro ao abrir arquivo");
        return;
    }

    fseek(file, 0, SEEK_END);

    char buff[63];
    int bytesRead = 0;
    int sequence = 0;
    while ((bytesRead = fread(buff, 1, 63, file)) > 0) {
        Message* msg = createMessage(bytesRead, sequence, DATA, buff, 0);
        rawSocketSend(rsocket, msg, sizeof(Message), 0);
        sequence++;
    }

    fclose(file);
}

int main() {
    int rsocket = rawSocketCreator("eno1");

    while (1) {
        Message* receivedBytes = receiveMessage(rsocket);

        if (receivedBytes == NULL) {
            perror("Erro ao receber mensagem");
        }

        printf("Mensagem recebida: %s\n", receivedBytes->data);

        switch (receivedBytes->type)
        {
            case DOWNLOAD:
                sendFile(rsocket, "README.md");
                break;
        }
    }

    return 0;
}
