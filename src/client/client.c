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

void handle_data(int rsocket, Message *receivedBytes, long int *expectedSequence, int *sent_first_byte, char *filename)
{
    if (calculateCRC8(receivedBytes->data, receivedBytes->size) != receivedBytes->error && receivedBytes->error != 0x00)
    {
        Message *nack = createMessage(17, receivedBytes->sequence, NACK, (uint8_t *)"Not Acknowledged");
        sendMessage(rsocket, nack);
        free(nack);
    }
    else if (receivedBytes->sequence == (*expectedSequence % MAX_SEQUENCE))
    {
        appendFile(filename, receivedBytes->data, receivedBytes->size);
        Message *ack = createMessage(13, receivedBytes->sequence, ACK, (uint8_t *)"Acknowledged");
        (*expectedSequence)++;
        *sent_first_byte = 1;
        sendMessage(rsocket, ack);
        free(ack);
    }
    else
    {
        Message *ack = createMessage(21, (*expectedSequence - 1) % MAX_SEQUENCE, ACK, (uint8_t *)"Out of order message");
        sendMessage(rsocket, ack);
        free(ack);
    }
}

void handle_end(int rsocket, Message *receivedBytes)
{
    Message *ack = createMessage(13, receivedBytes->sequence, ACK, (uint8_t *)"Acknowledged");
    sendMessage(rsocket, ack);
    free(ack);
    free(receivedBytes);
}

void handle_error(Message *receivedBytes)
{
    printf("Erro ao baixar arquivo: ");
    printf("%s\n", receivedBytes->data);
    free(receivedBytes);
}

void download_file(int rsocket, char *filename)
{
    printf("Baixando arquivo %s...\n", filename);
    Message *msg = createMessage(strlen(filename), 0, DOWNLOAD, (uint8_t *)filename);
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
                Message *msg = createMessage(strlen(filename), 0, DOWNLOAD, (uint8_t *)filename);
                sendMessage(rsocket, msg);
                free(msg);
            }
            retries++;
        }

        if (retries > MAX_RETRIES)
        {
            printf("Servidor não encontrado\n");
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
            handle_data(rsocket, receivedBytes, &expectedSequence, &sent_first_byte, filename);
            break;
        case END:
            handle_end(rsocket, receivedBytes);
            return;
        case ERROR:
            handle_error(receivedBytes);
            return;
        }
        free(receivedBytes);
    }

    return;
}

void handle_file_info(int rsocket, Message *receivedBytes)
{
    printf("%s\n", receivedBytes->data);
    Message *msg = createMessage(13, receivedBytes->sequence, ACK, (uint8_t *)"Acknowledged");
    sendMessage(rsocket, msg);
    free(msg);
}

void handle_show(int rsocket, Message *receivedBytes, int *has_sent_first_byte)
{
    if (calculateCRC8(receivedBytes->data, receivedBytes->size) != receivedBytes->error && receivedBytes->error != 0x00)
    {
        Message *nack = createMessage(17, receivedBytes->sequence, NACK, (uint8_t *)"Not Acknowledged");
        sendMessage(rsocket, nack);
        free(nack);
    }
    else
    {
        if (!*has_sent_first_byte)
        {
            printf("-=-=-=-=-=-=-=-=-=- Arquivos =-=-=-=-=-=-=-=-=-=\n");
        }
        printf("%s | ", receivedBytes->data);
        Message *ack = createMessage(13, receivedBytes->sequence, ACK, (uint8_t *)"Acknowledged");
        *has_sent_first_byte = 1;
        sendMessage(rsocket, ack);
        free(ack);
    }
}

void handle_end_list(int rsocket, Message *receivedBytes)
{
    Message *ack = createMessage(13, receivedBytes->sequence, ACK, (uint8_t *)"Acknowledged");
    sendMessage(rsocket, ack);
    printf("-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=\n");
    free(receivedBytes);
}

void handle_error_list(Message *receivedBytes)
{
    printf("Erro ao listar arquivos: ");
    printf("%s\n", receivedBytes->data);
    free(receivedBytes);
}

void handle_list_option(int rsocket)
{
    Message *msg = createMessage(25, 0, LIST, (uint8_t *)"List all files in public");
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
                msg = createMessage(25, 0, LIST, (uint8_t *)"List all files in public");
                sendMessage(rsocket, msg);
            }
            retries++;
        }

        if (retries > MAX_RETRIES)
        {
            printf("Servidor não encontrado\n");
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
            handle_file_info(rsocket, receivedBytes);
            break;
        case SHOW:
            handle_show(rsocket, receivedBytes, &has_sent_first_byte);
            break;
        case END:
            handle_end_list(rsocket, receivedBytes);
            return;
        case ERROR:
            handle_error_list(receivedBytes);
            return;
        }

        free(receivedBytes);
    }

    return;
}

void handle_download_option(int rsocket)
{
    char *filename = malloc(FILENAME_MAX);
    memset(filename, '\0', FILENAME_MAX);

    printf("Digite o nome do arquivo: ");
    int r = scanf("\n%s", filename);

    if (r == EOF)
    {
        free(filename);
        return;
    }

    filename = realloc(filename, strlen(filename) + 1);
    if (strlen(filename) < 9 || strcmp(filename + strlen(filename) - 4, ".mp4") != 0)
    {
        printf("Nome de arquivo inválido\n");
        free(filename);
        return;
    }

    remove(filename);
    download_file(rsocket, filename);
    free(filename);
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        fprintf(stderr, "Uso: %s <interface>\n", argv[0]);
        return 1;
    }

    int rsocket = rawSocketCreator(argv[1]);
    if (rsocket == -1)
    {
        fprintf(stderr, "Erro ao criar socket\n");
        return -1;
    }

    printf("baixar aquivo (b)\n");
    printf("listar arquivos (l)\n");
    printf("sair (q)\n");
    printf("\nOpção: ");

    char *option = malloc(2 * sizeof(char));
    int r = scanf("%s", option);

    if (r == EOF)
    {
        free(option);
        return 0;
    }

    while (1)
    {
        switch (option[0])
        {
        case 'b':
            handle_download_option(rsocket);
            break;
        case 'l':
            handle_list_option(rsocket);
            break;
        case 'q':
            free(option);
            return 0;
        default:
            printf("Opção inválida\n");
            break;
        }
        printf("\nOpção: ");
        int r = scanf("\n%s", option);
        if (r == EOF)
        {
            free(option);
            return 0;
        }
    }

    return 0;
}
