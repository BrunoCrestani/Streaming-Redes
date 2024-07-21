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
    sleep(1); // wait for client to be ready
    FILE *file = fopen(filename, "rb");

    if (file == NULL)
    {
        fprintf(stderr, "Erro ao abrir arquivo: %s", filename);
        return;
    }

    char buff[MAX_DATA_SIZE];
    size_t bytesRead = 0;
    int sequence = 0;

    for (sequence = 0; sequence < WINDOW_SIZE; sequence++)
    {
        bytesRead = fread(buff, 1, MAX_DATA_SIZE, file);

        if (bytesRead == 0)
        {
            break;
        }

        Message *msg = createMessage(bytesRead, sequence % MAX_SEQUENCE, DATA, buff);
        enqueueMessage(msg);
    }

    long long timeoutMillis = 250; // 250ms
    long long start = timestamp(); // already timeouted

    struct timeval tv = { .tv_sec = timeoutMillis / 1000, .tv_usec = (timeoutMillis % 1000) * 1000 };
    setsockopt(rsocket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv); // set new options

    while (!isEmpty())
    {
        Message *receivedBytes = receiveMessage(rsocket);

        if (timestamp() - start >= timeoutMillis)
        {
            printf("Timeout\n");
            sendQueue(rsocket);
            start = timestamp();
        }

        if (receivedBytes == NULL)
        {
            continue;
        }


        if (receivedBytes->type == ACK)
        {
            Message *firstOfWindow = peekMessage();

            if (firstOfWindow == NULL)
            {
                break;
            }

            if (receivedBytes->sequence == firstOfWindow->sequence)
            {
                printf("Movendo janela...\n");
                start = timestamp();
                dequeueMessage();
                
                bytesRead = fread(buff, 1, MAX_DATA_SIZE, file);

                if (bytesRead == 0)
                {
                    continue; // EOF
                }

                Message *msg = createMessage(bytesRead, sequence % MAX_SEQUENCE, DATA, buff);
                sequence++;
                enqueueMessage(msg);
                sendMessage(rsocket, msg);                
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
            const char *filepath = "public/";
            char* filename = malloc(receivedBytes->size + strlen(filepath) + 1);

            strcat(filename, filepath);
            strcat(filename, receivedBytes->data);
            strcat(filename, "\0");
            
            sendFile(rsocket, filename);
            break;
        }
    }

    return 0;
}
