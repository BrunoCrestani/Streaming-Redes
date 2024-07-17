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

int main() {
    int rsocket = rawSocketCreator("lo");

    while (1) {
        Message* receivedBytes = receiveMessage(rsocket);

        if (receivedBytes == NULL) {
            perror("Erro ao receber mensagem");
            exit(-1);
        }

        printf("Mensagem recebida: %s\n", receivedBytes->data);
    }

    close(rsocket);
    return 0;
}