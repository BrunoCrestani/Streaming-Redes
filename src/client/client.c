#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <net/ethernet.h> 
#include <linux/if_packet.h> 
#include <sys/types.h>
#include <sys/socket.h>
#include <stddef.h> 
#include <net/if.h> 
#include <unistd.h>
#include <arpa/inet.h> 
#include <unistd.h>
#include <errno.h>
#include "../raw_sockets/sockets.h"
#include "../message/message.h"

int main() {
    int rsocket = rawSocketCreator("enp3s0f3u3");
    Message* msg = createFakeMessage();

    while (1) {
        int sentBytes = sendMessage(rsocket, msg);

        if (!sentBytes) {
            perror("Erro ao enviar mensagem");
        }
    }

    return 0;
}