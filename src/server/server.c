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
#include <sys/time.h>
#include "../raw_sockets/sockets.h"
#include "../message/message.h"

long long timestamp() {
    struct timeval tp;
    gettimeofday(&tp, NULL);
    return tp.tv_sec*1000 + tp.tv_usec/1000;
}

void sendFile(int rsocket, char *filename)
{
    FILE *file = fopen(filename, "r");

    if (file == NULL)
    {
        perror("Erro ao abrir arquivo");
        return;
    }

    char buff[63];
    int bytesRead = 0;
    int sequence = 0;
    const int windowSize = 4;

    for (int i = 0; i < windowSize; i++)
    {
        bytesRead = fread(buff, 1, 63, file);

        if (bytesRead == 0)
        {
            break;
        }

        Message *msg = createMessage(bytesRead, sequence % windowSize, DATA, buff);
        enqueueMessage(msg);
        sequence++;
    }

    printQueue();

    long long start = timestamp();
    sendQueue(rsocket);
    long long timeoutMillis = 2000;

    struct timeval tv = { .tv_sec = timeoutMillis / 1000, .tv_usec = (timeoutMillis % 1000) * 1000 };
    setsockopt(rsocket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv); // set new options

    while (1)
    {
        Message *receivedBytes = receiveMessage(rsocket);

        printf("%lld", timestamp() - start);

        if (timestamp() - start > timeoutMillis)
        {
            printf("Timeout\n");
            sendQueue(rsocket);
            start = timestamp();
        }

        if (receivedBytes == NULL)
        {
            fprintf(stderr, "Erro ao receber mensagem\n");
            continue;
        }

        if (receivedBytes->type == ACK)
        {
            printQueue();

            Message *firstOfWindow = peekMessage();

            if (firstOfWindow == NULL)
            {
                break;
            }

            if (receivedBytes->sequence == firstOfWindow->sequence)
            {
                printf("Movendo janela...\n");
                start = timestamp();
                Message* toSend = dequeueMessage();
                
                sendMessage(rsocket, toSend);                

                bytesRead = fread(buff, 1, 63, file);

                if (bytesRead == 0)
                {
                    continue; // EOF
                }

                Message *msg = createMessage(bytesRead, sequence % windowSize, DATA, buff);
                enqueueMessage(msg);
                sequence++;
            }
        }
    }

    printf("Arquivo enviado\n");

    fclose(file);
}

int main()
{
    int rsocket = rawSocketCreator("eno1");

    while (1)
    {
        Message *receivedBytes = receiveMessage(rsocket);

        if (receivedBytes == NULL)
        {
            // perror("Erro ao receber mensagem");
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
