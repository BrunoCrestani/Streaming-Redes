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
#include <unistd.h>
#include "../raw_sockets/sockets.h"
#include "../message/message.h"

void sendFile(int rsocket, char* filename) {
    sleep(5); // Espera o cliente começar a escutar
    FILE* file = fopen(filename, "r");

    if (file == NULL) {
        perror("Erro ao abrir arquivo");
        return;
    }

    char buff[63];
    int bytesRead = 0;
    int sequence = 0;
    sendMessage(rsocket, createMessage(strlen(filename) + 1, sequence, FILE_INFO, filename));

    while ((bytesRead = fread(buff, 1, 63, file)) > 0) {
        sequence++;
        printf("Enviando %s\n", buff);
        Message*msg = createMessage(bytesRead, sequence, DATA, buff);
        sendMessage(rsocket, msg);
    }

    fclose(file);
}

int main() {
    int rsocket = rawSocketCreator("eno1");

    while (1) {
        Message* receivedBytes = receiveMessage(rsocket);

        if (receivedBytes == NULL) {
            perror("Erro ao receber mensagem");
            continue;
        }

        printf("Mensagem recebida: %s\n", receivedBytes->data);

        switch (receivedBytes->type)
        {
            case DOWNLOAD:
                printf("Enviando arquivo\n");
                sendFile(rsocket, "README.md");
                break;
        }
    }

    return 0;
}
