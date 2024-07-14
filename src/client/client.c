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
    unsigned int rsocket = rawSocketCreator("lo");

    Message* message = createMessage(6, 1, 2, "Hello", 0);

    struct sockaddr_ll server_addr = {0};
    server_addr.sll_family = AF_PACKET;
    server_addr.sll_protocol = htons(ETH_P_ALL);

    // Use loopback interface
    server_addr.sll_ifindex = if_nametoindex("lo");
    if (server_addr.sll_ifindex == 0) {
        perror("Erro ao obter index da interface");
        exit(-1);
    }

    if (sendto(rsocket, message, sizeof(Message), 0, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("Erro ao enviar mensagem");
        exit(-1);
    }

    printf("Mensagem enviada\n");

    close(rsocket);
    return 0;
}