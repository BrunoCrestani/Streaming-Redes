#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <dirent.h>
#include <unistd.h>

#include "message.h"
#include "./../raw_sockets/sockets.h"
#include "./../media/media.h"

/*
 * Initializes a message queue
 */
void initQueue(MessageQueue *queue)
{
  queue->head = NULL;
  queue->tail = NULL;
}

/*
 * Puts a new message on the queue
 */
void enqueueMessage(MessageQueue *queue, Message *msg)
{
  MessageNode *node = malloc(sizeof(MessageNode));
  node->message = msg;
  node->next = NULL;

  if (queue->tail == NULL)
  {
    queue->head = queue->tail = node;
  }
  else
  {
    queue->tail->next = node;
    queue->tail = node;
  }
}

/*
 * Gets a message out of the queue
 */
Message *dequeueMessage(MessageQueue *queue)
{
  if (queue->head == NULL)
    return NULL;

  MessageNode *temp = queue->head;
  Message *msg = queue->head->message;
  queue->head = queue->head->next;
  if (queue->head == NULL)
    queue->tail = NULL;
  free(temp);
  return msg;
}

Message *peekMessage(MessageQueue *queue)
{
  if (queue->head == NULL)
    return NULL;
  return queue->head->message;
}

void sendQueue(MessageQueue *queue, int sockfd)
{
  MessageNode *temp = queue->head;
  while (temp != NULL)
  {
    int sentBytes = sendMessage(sockfd, temp->message);
    temp = temp->next;
  }
}

int isEmpty(MessageQueue *queue)
{
  return queue->head == NULL;
}

long long timestamp()
{
  struct timeval tp;
  gettimeofday(&tp, NULL);
  return tp.tv_sec * 1000 + tp.tv_usec / 1000;
}
/*
 * Creates a message
 */
Message *createMessage(uint8_t size, uint8_t sequence, uint8_t type, uint8_t data[])
{
  Message *msg = malloc(sizeof(Message));

  if (!msg)
    return NULL;

  memset(msg, 0, sizeof(Message));
  msg->marker = INIT_MARKER;
  msg->size = size;
  msg->sequence = sequence;
  msg->type = type;
  memcpy(msg->data, data, size);
  msg->error = calculateCRC8(data, size);

  return msg;
}

/*
 * The process of sending a
 * message from the server to the user
 */
int sendMessage(int sockfd, Message *msg)
{
  if (msg == NULL)
    return -1;

  size_t messageSize = sizeof(Message) - MAX_DATA_SIZE + msg->size;
  uint8_t buffer[messageSize];
  memcpy(buffer, msg, messageSize);
  unsigned int sentBytes = rawSocketSend(sockfd, buffer, messageSize, 0);

  return sentBytes;
}

/*
 * The process of receiving a message from the user
 * to the server
 */
Message *receiveMessage(int sockfd)
{
  uint8_t buffer[sizeof(Message)];
  memset(buffer, 0, sizeof(Message));

  ssize_t receivedBytes = recv(sockfd, buffer, sizeof(Message), 0);

  Message *msg = malloc(sizeof(Message));
  memset(msg, 0, sizeof(Message));

  if (receivedBytes <= 0 || msg == NULL)
    return NULL;

  memcpy(msg, buffer, receivedBytes);
  if (msg->marker != INIT_MARKER)
    return NULL;
  return msg;
}

/*
 * Calculates CRC-8 (code is being used as example)
 */
uint8_t calculateCRC8(const uint8_t *data, uint8_t len)
{
  uint8_t crc = 0xFF;
  for (size_t i = 0; i < len; i++)
  {
    crc ^= data[i];
    for (uint8_t j = 0; j < 8; j++)
    {
      if (crc & 0x80 != 0)
      {
        // Example
        crc = (uint8_t)((crc << 1) ^ 0x31);
      }
      else
      {
        crc <<= 1;
      }
    }
  }
  return crc;
}

void sendMessageStopAndWait(int sockfd, Message *msg, long long timeoutMillis, int maxRetries) {
    struct timeval tv = {.tv_sec = timeoutMillis / 1000, .tv_usec = (timeoutMillis % 1000) * 1000};
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char *)&tv, sizeof tv);
    
    long long start = timestamp();
    int retries = 0;

    while (1) {
        Message *receivedBytes = receiveMessage(sockfd);

        if (timestamp() - start > timeoutMillis) {
            sendMessage(sockfd, msg);
            start = timestamp();
            retries++;
        }

        if (retries > maxRetries) {
            printf("Número máximo de tentativas atingido\n");
            break;
        }

        if (receivedBytes == NULL) {
            continue;
        }

        retries = 0;

        if (receivedBytes->type == ACK && receivedBytes->sequence == msg->sequence) {
            break;
        } else if (receivedBytes->type == NACK) {
            start = timestamp();
            sendMessage(sockfd, msg);
        }
        free(receivedBytes);
    }
}

/*
 * When an LIST is received the
 * media array is displayed in the UI
 *
 */
void listHandler(int sockfd)
{
  printf("Enviando lista de arquivos\n");
  const char *filepath = "public/";

  // stop and wait protocol, so we can send the files one by one

  DIR *d;
  struct dirent *dir;
  d = opendir(filepath);

  if (!d)
  {
    printf("Erro ao abrir diretório\n");
    return;
  }

  const long long timeoutMillis = 250; // 250ms
  const int MAX_RETRIES = 24;          // 250ms * 24 = 6s

  long long start = timestamp();

  while ((dir = readdir(d)) != NULL)
  {
    if (strlen(dir->d_name) < 6 || dir->d_type != DT_REG)
    {
      continue;
    }

    Message *msg = createMessage(strlen(dir->d_name) + 1, 0, SHOW, strcat(dir->d_name, "\0"));
    
    sendMessage(sockfd, msg);
    sendMessageStopAndWait(sockfd, msg, timeoutMillis, MAX_RETRIES);

    free(msg);

    Media *media = media_new();

    char *path = malloc(strlen(filepath) + strlen(dir->d_name) + 1);
    memset(path, '\0', strlen(filepath) + strlen(dir->d_name) + 1);
    strcat(path, filepath);
    strcat(path, dir->d_name);

    if (media_stat(path, media) == -1)
    {
      printf("Erro ao obter informações do arquivo\n");
      continue;
    }

    char* media_str = media_to_string(media);
    Message *fileInfo = createMessage(strlen(media_str), 0, FILE_INFO, media_str);
    sendMessage(sockfd, fileInfo);
    sendMessageStopAndWait(sockfd, fileInfo, timeoutMillis, MAX_RETRIES);

    free(path);
    free(media);
    free(fileInfo);
    free(media_str);
  }

  // send end message
  Message *msg = createMessage(19, 0, END, "Arquivos enviados");
  sendMessage(sockfd, msg);

  sendMessageStopAndWait(sockfd, msg, timeoutMillis, MAX_RETRIES);

  free(msg);
  closedir(d);
}

int is_readable_by_others(const char *path)
{
  struct stat st;

  if (stat(path, &st) != 0)
  {
    return 0;
  }

  return (st.st_mode & S_IROTH) != 0;
}

int is_in_public_and_is_mp4(const char *path)
{
  return strstr(path, "public/") == path && strstr(path, ".mp4") != NULL;
}

int is_disk_full(const char *path)
{
  struct statvfs buf;

  if (statvfs(path, &buf) < 0)
  {
    return 0;
  }

  unsigned long free_space = buf.f_bsize * buf.f_bavail / 1024 / 1024;

  return free_space < 10;
}
/*
 * When a DOWNLOAD is received
 * the file is sent to the user in chunks using a window
 */
void downloadHandler(Message *receivedBytes, int sockfd)
{
  printf("Enviando arquivo\n");
  const char *filepath = "public/";
  char *filename = malloc(receivedBytes->size + strlen(filepath) + 1);
  memset(filename, '\0', receivedBytes->size + strlen(filepath) + 1);

  strcat(filename, filepath);
  strcat(filename, receivedBytes->data);

  FILE *file = fopen(filename, "rb");

  MessageQueue *queue = malloc(sizeof(MessageQueue));
  initQueue(queue);

  // check file permission
  if (!is_readable_by_others(filename))
  {
    sendMessage(sockfd, createMessage(24, ACCESS_DENIED, ERROR, "Acesso negado"));

    return;
  }

  if (is_disk_full(filepath))
  {
    sendMessage(sockfd, createMessage(24, DISK_IS_FULL, ERROR, "Disco cheio"));

    return;
  }

  if (file == NULL || !is_in_public_and_is_mp4(filename))
  {
    fprintf(stderr, "Erro ao abrir arquivo: %s", filename);

    sendMessage(sockfd, createMessage(24, NOT_FOUND, ERROR, "Arquivo não encontrado"));

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
    enqueueMessage(queue, msg);
  }

  const long long timeoutMillis = 250; // 250ms
  const int MAX_RETRIES = 24;          // 250ms * 24 = 6s

  struct timeval tv = {.tv_sec = timeoutMillis / 1000, .tv_usec = (timeoutMillis % 1000) * 1000};
  setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char *)&tv, sizeof tv); // set new options

  long long start = timestamp();
  int retries = 0;

  sendQueue(queue, sockfd);

  while (!isEmpty(queue))
  {
    Message *receivedBytes = receiveMessage(sockfd);

    if (timestamp() - start > timeoutMillis)
    {
      sendQueue(queue, sockfd);
      start = timestamp();
      retries++;
    }

    if (retries > MAX_RETRIES)
    {
      printf("Número máximo de tentativas atingido\n");
      // free queue
      while (!isEmpty(queue))
      {
        Message *msg = dequeueMessage(queue);
        free(msg);
      }
      free(queue);

      free(filename);
      fclose(file);
      return;
    }

    if (receivedBytes == NULL)
    {
      continue;
    }

    retries = 0;

    if (receivedBytes->type == ACK)
    {
      Message *firstOfWindow = peekMessage(queue);

      if (firstOfWindow == NULL)
      {
        break;
      }

      if (receivedBytes->sequence == firstOfWindow->sequence)
      {
        start = timestamp();
        dequeueMessage(queue);

        bytesRead = fread(buff, 1, MAX_DATA_SIZE, file);

        if (bytesRead == 0)
        {
          continue;
        }

        Message *msg = createMessage(bytesRead, sequence % MAX_SEQUENCE, DATA, buff);
        sequence++;
        enqueueMessage(queue, msg);
        sendMessage(sockfd, msg);
      }
    }
    else if (receivedBytes->type == NACK)
    {
      start = timestamp();
      sendQueue(queue, sockfd);
    }
  }

  Message *msg = createMessage(19, 0, END, "Arquivos enviados");
  sendMessage(sockfd, msg);

  const long long ackLostTimeout = 3000; // 3s

  sendMessageStopAndWait(sockfd, msg, ackLostTimeout, MAX_RETRIES);

  printf("Arquivo enviado\n");

  // free queue
  while (!isEmpty(queue))
  {
    Message *msg = dequeueMessage(queue);
    free(msg);
  }
  free(queue);

  free(filename);
  free(msg);
  fclose(file);
}

/*
 * Redirects the message received,
 * using its type to get it properly
 * handled
 */
void answerHandler(Message *msg, int sockfd)
{
  switch (msg->type)
  {
  case LIST:
    listHandler(sockfd);

    break;
  case DOWNLOAD:
    downloadHandler(msg, sockfd);

    break;
  default:
    printf("invalid message type: %u\n", msg->type);
    break;
  }
}
