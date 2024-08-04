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
#include <sys/stat.h>
#include "../raw_sockets/sockets.h"
#include "../media/media.h"
#include "../message/message.h"

void appendFile(char *filename, uint8_t *data, uint8_t size)
{
    FILE *file = fopen(filename, "ab");

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
    printf("Baixando arquivo %s...\n", filename);
    Message *msg = createMessage(strlen(filename), 0, DOWNLOAD, filename);
    long int expectedSequence = 0;
    int sentBytes = sendMessage(rsocket, msg);
    free(msg);

    if (!sentBytes)
    {
        fprintf(stderr, "Erro ao enviar mensagem");
    }

    const long long TIMEOUT = 350; // 350ms
    const int MAX_RETRIES = 16;    // 16*350ms = 5.6s

    struct timeval tv = {.tv_sec = TIMEOUT / 1000, .tv_usec = (TIMEOUT % 1000) * 1000};
    setsockopt(rsocket, SOL_SOCKET, SO_RCVTIMEO, (const char *)&tv, sizeof(struct timeval));

    long long start = timestamp();
    int sent_first_byte = 0;
    int retries = 0;

    while (1)
    {
        Message *receivedBytes = receiveMessage(rsocket);

        if (timestamp() - start > TIMEOUT) // timeout on download
        {
            start = timestamp();
            if (expectedSequence == 0 && !sent_first_byte)
            {
                printf("Procurando servidor...\n");
                Message *msg = createMessage(strlen(filename), 0, DOWNLOAD, filename);
                sendMessage(rsocket, msg);
                free(msg);
            }
            retries++;
        }

        if (retries > MAX_RETRIES)
        {
            printf("Servidor não encontrado\n");
            free(msg);
            free(receivedBytes);
            remove(filename);
            return;
        }

        if (receivedBytes == NULL)
        {
            continue;
        }

        retries = 0;

        switch (receivedBytes->type)
        {
        case DATA:
            if (calculateCRC8(receivedBytes->data, receivedBytes->size) != receivedBytes->error && receivedBytes->error != 0x00)
            {
                Message *nack = createMessage(17, receivedBytes->sequence, NACK, "Not Acknowledged");
                sendMessage(rsocket, nack);
                free(nack);
            }
            else if (receivedBytes->sequence == (expectedSequence % MAX_SEQUENCE))
            {
                appendFile(filename, receivedBytes->data, receivedBytes->size);
                Message *ack = createMessage(13, receivedBytes->sequence, ACK, "Acknowledged");
                expectedSequence++;
                sent_first_byte = 1;
                sendMessage(rsocket, ack);
                free(ack);
            }
            else
            {
                Message *ack = createMessage(21, (expectedSequence - 1) % MAX_SEQUENCE, ACK, "Out of order message");
                sendMessage(rsocket, ack);
                free(ack);
            }

            break;

        case END:
            Message *ack = createMessage(13, receivedBytes->sequence, ACK, "Acknowledged");
            sendMessage(rsocket, ack);
            free(ack);
            free(receivedBytes);
            return;
            break;
        case ERROR:
            printf("Erro ao baixar arquivo: ");
            printf("%s\n", receivedBytes->data);
            free(receivedBytes);
            return;
        }
        free(receivedBytes);
    }

    return;
}

void print_files(int rsocket)
{
    Message *msg = createMessage(25, 0, LIST, "List all files in public");
    int sentBytes = sendMessage(rsocket, msg);
    free(msg);
    if (!sentBytes)
    {
        fprintf(stderr, "Erro ao enviar mensagem");
    }

    const long long TIMEOUT = 1000; // 1 second
    const int MAX_RETRIES = 10;

    struct timeval tv = {.tv_sec = TIMEOUT / 1000, .tv_usec = (TIMEOUT % 1000) * 1000};
    setsockopt(rsocket, SOL_SOCKET, SO_RCVTIMEO, (const char *)&tv, sizeof(struct timeval));

    long long start = timestamp();
    int has_sent_first_byte = 0;
    int retries = 0;

    while (1)
    {
        Message *receivedBytes = receiveMessage(rsocket);

        if (timestamp() - start > TIMEOUT) // timeout on download
        {
            start = timestamp();
            if (!has_sent_first_byte)
            {
                printf("Procurando servidor...\n");
                msg = createMessage(25, 0, LIST, "List all files in public");
                sendMessage(rsocket, msg);
            }
            retries++;
        }

        if (retries > MAX_RETRIES)
        {
            printf("Servidor não encontrado\n");
            free(msg);
            free(receivedBytes);
            return;
        }

        if (receivedBytes == NULL)
        {
            continue;
        }

        retries = 0;

        switch (receivedBytes->type)
        {
        case FILE_INFO:
            printf("%s\n", receivedBytes->data);
            Message *msg = createMessage(13, receivedBytes->sequence, ACK, "Acknowledged");
            sendMessage(rsocket, msg);
            free(msg);

            break;
        case SHOW:
            if (calculateCRC8(receivedBytes->data, receivedBytes->size) != receivedBytes->error && receivedBytes->error != 0x00)
            {
                Message *nack = createMessage(17, receivedBytes->sequence, NACK, "Not Acknowledged");
                sendMessage(rsocket, nack);
                free(nack);
            }
            else
            {
                if (!has_sent_first_byte)
                {
                    printf("-=-=-=-= Arquivos -=-=-=-=\n");
                }
                printf("%s | ", receivedBytes->data);
                Message *ack = createMessage(13, receivedBytes->sequence, ACK, "Acknowledged");
                has_sent_first_byte = 1;
                sendMessage(rsocket, ack);
                free(ack);
            }
            break;
        case END:
            Message *ack = createMessage(13, receivedBytes->sequence, ACK, "Acknowledged");
            sendMessage(rsocket, msg);
            printf("-=-=-=-=-=-=-=-=-=-=-=-=-=\n");
            free(receivedBytes);
            free(msg);
            return;
            break;
        case ERROR:
            printf("Erro ao listar arquivos: ");
            printf("%s\n", receivedBytes->data);
            free(receivedBytes);
            return;
        }

        free(receivedBytes);
    }

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

    printf("baixar aquivo (b)\n");
    printf("listar arquivos (l)\n");
    printf("sair (q)\n");
    printf("\nOpção: ");

    char *option = malloc(2);
    scanf("%s", option);

    while (1)
    {
        switch (option[0])
        {
        case 'b':
            char *filename = malloc(FILENAME_MAX);
            memset(filename, '\0', FILENAME_MAX);

            printf("Digite o nome do arquivo: ");
            scanf("\n%s", filename);
            filename = realloc(filename, strlen(filename) + 1);
            if (strlen(filename) < 9 || strcmp(filename + strlen(filename) - 4, ".mp4") != 0)
            {
                printf("Nome de arquivo inválido\n");
                free(filename);
                break;
            }

            remove(filename);
            download_file(rsocket, filename);
            free(filename);

            break;
        case 'l':
            print_files(rsocket);
            break;
        case 'q':
            free(option);
            return 0;
        default:
            printf("Opção inválida\n");
            break;
        }
        printf("\nOpção: ");
        scanf("\n%s", option);
    }

    return 0;
}