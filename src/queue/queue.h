#include "../message/message.h"

typedef struct MessageNode
{
  Message *message;
  struct MessageNode *next;

} MessageNode;

typedef struct MessageQueue
{
  MessageNode *head;
  MessageNode *tail;
} MessageQueue;

void initQueue(MessageQueue *queue);
void enqueueMessage(MessageQueue *queue, Message *msg);
Message *dequeueMessage(MessageQueue *queue);
Message *peekMessage(MessageQueue *queue);
void sendQueue(MessageQueue *queue, int sockfd);
int isEmpty(MessageQueue *queue);
