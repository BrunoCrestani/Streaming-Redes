#include <stdio.h>
#include <stdlib.h>
#include "queue.h"

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
    if (sentBytes == -1)
    {
      fprintf(stderr, "Erro ao enviar mensagem\n");
      continue;
    }
    temp = temp->next;
  }
}

int isEmpty(MessageQueue *queue)
{
  return queue->head == NULL;
}
