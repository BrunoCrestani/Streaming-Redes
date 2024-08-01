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
#include <sys/time.h>
#include "../raw_sockets/sockets.h"
#include "../message/message.h"

void appendFile(char *filename, uint8_t *data, uint8_t size)
{
    FILE *file = fopen(filename, "ab+");

    if (file == NULL)
    {
        perror("Erro ao abrir arquivo");
        return;
    }

    fwrite(data, 1, size, file);

    fclose(file);
}

int main()
{
    int rsocket = rawSocketCreator("enp3s0f3u3");

    if (rsocket == -1)
    {
        fprintf(stderr, "Erro ao criar socket\n");
        return -1;
    }

    Message *msg = createMessage(11, 0, DOWNLOAD, "mewing.mp3");
    long int expectedSequence = 0;
    int sentBytes = sendMessage(rsocket, msg);

    if (!sentBytes)
    {
        fprintf(stderr, "Erro ao enviar mensagem");
    }

    long long timeout = 1500;
    struct timeval tv = {.tv_sec = timeout / 1000, .tv_usec = (timeout % 1000) * 1000};
    setsockopt(rsocket, SOL_SOCKET, SO_RCVTIMEO, (const char *)&tv, sizeof(struct timeval));
    long long start = timestamp();

    while (1)
    {
        Message *receivedBytes = receiveMessage(rsocket);

        if (timestamp() - start > timeout) // timeout on download
        {
            start = timestamp();
            if (expectedSequence == 0)
            {
                Message *msg = createMessage(11, 0, DOWNLOAD, "mewing.mp3");
                sendMessage(rsocket, msg);
            }
        }

        if (receivedBytes == NULL)
        {
            continue;
        }

        switch (receivedBytes->type)
        {
        case DATA:
            if (receivedBytes->sequence == (expectedSequence % MAX_SEQUENCE))
            {
                printf("Received message with sequence %d\n", receivedBytes->sequence);
                appendFile("mewing.mp3", receivedBytes->data, receivedBytes->size);
                Message* ack = createMessage(13, receivedBytes->sequence, ACK, "Acknowledged");
                expectedSequence++;
                sendMessage(rsocket, ack);
            } else {
                printf("Received out of order message\n");
                Message* ack = createMessage(21, (expectedSequence - 1) % MAX_SEQUENCE, ACK, "Out of order message");
                sendMessage(rsocket, ack);
            }

            // appendFile("README.md", receivedBytes->data, receivedBytes->size);
            break;

        case END:
            printf("Received END message\n");
            return 0;
            break;
        }
    }

    return 0;
}