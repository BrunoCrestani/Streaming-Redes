#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <dirent.h>
#include "message.h"
#include "./../raw_sockets/sockets.h"

messageQueue *head = NULL;
messageQueue *tail = NULL;

/*
 * Puts a new message on the queue
 */
void enqueueMessage(Message *msg)
{
  messageQueue *node = (messageQueue *)malloc(sizeof(messageQueue));

  node->message = msg;
  node->next = NULL;

  if (tail == NULL)
  {
    head = tail = node;
  }
  else
  {
    tail->next = node;
    tail = node;
  }
}
/*
 * Gets a message out of the queue
 */
Message *dequeueMessage()
{
  if (head == NULL)
    return NULL;

  messageQueue *temp = head;
  Message *msg = head->message;
  head = head->next;
  if (head == NULL)
    tail = NULL;
  free(temp);
  return msg;
}

Message *peekMessage()
{
  if (head == NULL)
    return NULL;
  return head->message;
}

void sendQueue(int sockfd)
{
  messageQueue *temp = head;
  while (temp != NULL)
  {
    int sentBytes = sendMessage(sockfd, temp->message);
    temp = temp->next;
  }
}

void printQueue()
{
  messageQueue *temp = head;
  while (temp != NULL)
  {
    printf("-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=\n");
    printf("Message: %s\n Sequence: %d\n", temp->message->data, temp->message->sequence);
    printf("-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=\n");
    temp = temp->next;
  }
}

int isEmpty()
{
  return head == NULL;
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
  Message *msg;

  if (!(msg = malloc(sizeof(Message))))
    return NULL;

  msg->marker = INIT_MARKER;
  msg->size = size;
  msg->sequence = sequence;
  msg->type = type;
  memcpy(msg->data, data, size);
  msg->error = calculateCRC8(data, size);

  return msg;
}

Message *createFakeMessage()
{
  Message *msg;

  if (!(msg = malloc(sizeof(Message))))
    return NULL;

  msg->marker = INIT_MARKER;
  msg->size = 10;
  msg->sequence = 0;
  msg->type = DOWNLOAD;
  memcpy(msg->data, "README.md", 10);
  msg->error = calculateCRC8("README.md", 10);

  return msg;
}

/*
 * Deletes a fully acknowledged message
 */
void deleteMessage(Message *msg, unsigned int sizeAck)
{
  if (sizeAck == msg->size)
    free(msg);
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

  Message *msg = (Message *)malloc(sizeof(Message));

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

/*
 * When an ACK is received, it will
 * call the next  message
 */
void ackHandler(Message *msg, int sockfd)
{
  if (msg == NULL)
  {
    printf("NULL message received.\n");
    return;
  }
  printf("Ack received to message ID: %d\n", msg->sequence);

  deleteMessage(msg, msg->size);

  Message *nextMsg = dequeueMessage();

  if (nextMsg != NULL)
  {
    int sockfd = 0;

    if (sendMessage(sockfd, nextMsg) == 0)
    {
      printf("Next message sent: ID: %d\n", nextMsg->sequence);
    }
  }
}

/*
 * When an NACK is received the frame
 * in which the NACK was found is re-sent
 */
void nackHandler(Message *msg, int sockfd)
{
  if (msg == NULL)
  {
    printf("NULL message received.\n");
    return;
  }

  printf("NACK received to message ID: %d\n", msg->sequence);

  msg = dequeueMessage();

  if (msg != NULL)
  {
    int sockfd = 0;

    if (sendMessage(sockfd, msg) == 0)
    {
      printf("Message re-sent: ID: %d\n", msg->sequence);
    }
  }
}

/*
 * When an LIST is received the
 * media array is displayed in the UI
 *
 */
void listHandler(Message *_, int sockfd)
{
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

  long long timeoutMillis = 250; // 250ms
  long long start = timestamp();
  struct timeval tv = {.tv_sec = timeoutMillis / 1000, .tv_usec = (timeoutMillis % 1000) * 1000};

  while ((dir = readdir(d)) != NULL)
  {
    if (strlen(dir->d_name) < 6 || dir->d_type != DT_REG)
    {
      continue;
    }

    Message *msg = createMessage(strlen(dir->d_name) + 1, 0, SHOW, strcat(dir->d_name, "\0"));
    sendMessage(sockfd, msg);

    while (1)
    {
      Message* receivedBytes = receiveMessage(sockfd);

      if (timestamp() - start > timeoutMillis)
      {
        printf("Timeout\n");
        sendMessage(sockfd, msg);
        start = timestamp();
      }

      if (receivedBytes == NULL)
      {
        continue;
      }

      if (receivedBytes->type == ACK)
      {
        break;
      }
      else if (receivedBytes->type == NACK)
      {
        sendMessage(sockfd, msg);
      }
    }

  }

  // send end message

  Message *msg = createMessage(19, 0, END, "Arquivos enviados");
  sendMessage(sockfd, msg);
  const long long ackLostTimeout = 3000; // 3s

  long long startAckLostTimeout = timestamp();
  start = timestamp();

  while (1)
  {
    Message *receivedBytes = receiveMessage(sockfd);

    if (timestamp() - start > timeoutMillis)
    {
      printf("Timeout\n");
      sendMessage(sockfd, msg);
      start = timestamp();
    }

    if (timestamp() - startAckLostTimeout > ackLostTimeout)
    {
      printf("Arquivo enviado\n");
      break;
    }

    if (receivedBytes == NULL)
    {
      continue;
    }

    
    if (receivedBytes->type == ACK)
    {
      break;
    }
    else if (receivedBytes->type == NACK)
    {
      start = timestamp();
      sendMessage(sockfd, msg);
    }
  }

  free(msg);
  closedir(d);
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

  if (file == NULL)
  {
    fprintf(stderr, "Erro ao abrir arquivo: %s", filename);

    sendMessage(sockfd, createMessage(24, 0, ERROR, "Arquivo não encontrado"));

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

  const long long timeoutMillis = 250; // 250ms
  long long start = timestamp();

  struct timeval tv = {.tv_sec = timeoutMillis / 1000, .tv_usec = (timeoutMillis % 1000) * 1000};
  setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char *)&tv, sizeof tv); // set new options
  sendQueue(sockfd);

  while (!isEmpty())
  {
    Message *receivedBytes = receiveMessage(sockfd);

    if (timestamp() - start > timeoutMillis)
    {
      printf("Timeout\n");
      sendQueue(sockfd);
      start = timestamp();
    }

    if (receivedBytes == NULL)
    {
      continue;
    }

    start = timestamp();

    if (receivedBytes->type == ACK)
    {
      Message *firstOfWindow = peekMessage();

      if (firstOfWindow == NULL)
      {
        break;
      }

      if (receivedBytes->sequence == firstOfWindow->sequence)
      {
        dequeueMessage();

        bytesRead = fread(buff, 1, MAX_DATA_SIZE, file);

        if (bytesRead == 0)
        {
          continue;
        }

        Message *msg = createMessage(bytesRead, sequence % MAX_SEQUENCE, DATA, buff);
        sequence++;
        enqueueMessage(msg);
        sendMessage(sockfd, msg);
      }
    }
    else if (receivedBytes->type == NACK)
    {
      sendQueue(sockfd);
    }
  }

  Message *msg = createMessage(19, 0, END, "Arquivos enviados");
  sendMessage(sockfd, msg);

  const long long ackLostTimeout = 3000; // 3s

  while (1)
  {
    Message *receivedBytes = receiveMessage(sockfd);

    if (timestamp() - start > timeoutMillis)
    {
      printf("Timeout\n");
      sendMessage(sockfd, msg);
      start = timestamp();
    }

    if (timestamp() - start > ackLostTimeout)
    {
      printf("Arquivo enviado\n");
      break;
    }

    if (receivedBytes == NULL)
    {
      continue;
    }

    if (receivedBytes->type == ACK)
    {
      break;
    }
    else if (receivedBytes->type == NACK)
    {
      sendMessage(sockfd, msg);
    }
  }


  printf("Arquivo enviado\n");

  fclose(file);
}

/*
 * when a SHOW is received
 *
 */
void showHandler(Message *msg, int sockfd)
{
}

/*
 *
 */
void fileInfoHandler(Message *msg, int sockfd)
{
}

/*
 *
 */
void dataHandler(Message *msg, int sockfd)
{
}

/*
 *
 */
void endHandler(Message *msg, int sockfd)
{
}

/*
 *
 */
void errorHandler(Message *msg, int sockfd)
{
  switch (msg->error)
  {
  case ACCESS_DENIED:
    printf("Acess Denied");

    break;
  case NOT_FOUND:
    printf("Not Found");

    break;
  case DISK_IS_FULL:
    printf("User's disk is full. free %uMB before trying to download this title again.\n", msg->size);

    break;
  default:
    printf("Unknown Error type %u\n", msg->error);
  }
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
  case ACK:
    ackHandler(msg, sockfd);

    break;
  case NACK:
    nackHandler(msg, sockfd);

    break;
  case LIST:
    listHandler(msg, sockfd);

    break;
  case DOWNLOAD:
    downloadHandler(msg, sockfd);

    break;
  case SHOW:
    showHandler(msg, sockfd);

    break;
  case FILE_INFO:
    fileInfoHandler(msg, sockfd);

    break;
  case DATA:
    dataHandler(msg, sockfd);
    ;

    break;
  case END:
    endHandler(msg, sockfd);

    break;
  case ERROR:
    errorHandler(msg, sockfd);

    break;
  default:
    printf("invalid message type: %u\n", msg->type);
    break;
  }
}
