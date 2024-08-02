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

void download_file(int rsocket, char *filename)
{

    printf("Download iniciado\n");
    Message *msg = createMessage(strlen(filename), 0, DOWNLOAD, filename);
    long int expectedSequence = 0;
    int sentBytes = sendMessage(rsocket, msg);

    if (!sentBytes)
    {
        fprintf(stderr, "Erro ao enviar mensagem");
    }

    long long timeout = 1000; // 1s
    struct timeval tv = {.tv_sec = timeout / 1000, .tv_usec = (timeout % 1000) * 1000};
    setsockopt(rsocket, SOL_SOCKET, SO_RCVTIMEO, (const char *)&tv, sizeof(struct timeval));
    long long start = timestamp();
    int sent_first_byte = 0;

    while (1)
    {
        Message *receivedBytes = receiveMessage(rsocket);

        if (timestamp() - start > timeout) // timeout on download
        {
            start = timestamp();
            if (expectedSequence == 0 && !sent_first_byte)
            {
                printf("Procurando servidor...\n");
                Message *msg = createMessage(strlen(filename), 0, DOWNLOAD, filename);
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
            if (calculateCRC8(receivedBytes->data, receivedBytes->size) != receivedBytes->error && receivedBytes->error != 0x00)
            {
                Message *nack = createMessage(17, receivedBytes->sequence, NACK, "Not Acknowledged");
                sendMessage(rsocket, nack);
            }
            else if (receivedBytes->sequence == (expectedSequence % MAX_SEQUENCE))
            {
                appendFile(filename, receivedBytes->data, receivedBytes->size);
                Message *ack = createMessage(13, receivedBytes->sequence, ACK, "Acknowledged");
                expectedSequence++;
                sent_first_byte = 1;
                sendMessage(rsocket, ack);
            }
            else
            {
                Message *ack = createMessage(21, (expectedSequence - 1) % MAX_SEQUENCE, ACK, "Out of order message");
                sendMessage(rsocket, ack);
            }

            break;

        case END:
            Message *ack = createMessage(13, receivedBytes->sequence, ACK, "Acknowledged");
            sendMessage(rsocket, ack);
            printf("Download finalizado\n");
            return;
        case ERROR:
            printf("Erro ao baixar arquivo: ");
            printf("%s", receivedBytes->data);
            printf("\n");
            return;
        }
    }

    free(msg);

    return;
}

int main()
{
    int rsocket = rawSocketCreator("enp3s0f3u3");

    if (rsocket == -1)
    {
        fprintf(stderr, "Erro ao criar socket\n");
        return -1;
    }

    while (1)
    {
        char option;
        printf("baixar aquivo (b)\n");
        printf("listar arquivos (l)\n");
        printf("sair (q)\n");
        printf("\nOpção: ");
        scanf("%c", &option);

        switch (option)
        {
        case 'b':
            char *filename = malloc(FILENAME_MAX);
            memset(filename, '\0', FILENAME_MAX);

            printf("Digite o nome do arquivo: ");
            scanf("%s", filename);
            download_file(rsocket, filename);

            free(filename);

            break;
        case 'l':
            break;
        case 'q':
            return 0;
        default:
            printf("Opção inválida\n");
            break;
        }
    }

    return 0;
}