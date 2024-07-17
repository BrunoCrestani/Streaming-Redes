#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/socket.h>
#include "message.h"
#include "./../raw_sockets/sockets.h"


messageQueue* head = NULL;
messageQueue* tail = NULL;

/*
 * Puts a new message on the queue
 */
void enqueueMessage(Message *msg){
  messageQueue* node = (messageQueue*)malloc(sizeof(messageQueue));
  
  node->message = msg;
  node->next = NULL;

  if(tail == NULL){
    head = tail = node;
  } else {
    tail->next = node;
    tail = node;
  }
}

/*
 * Gets a message out of the queue
 */
Message* dequeueMessage(){
  if (head == NULL) return NULL;

    messageQueue* temp = head;
    Message* msg = head->message;
    head = head->next;
    if (head == NULL) tail = NULL;
    free(temp);
    return msg;
}

/*
 * Creates a message
 */
Message* createMessage(uint8_t size, uint8_t sequence, uint8_t type, uint8_t data[], uint8_t error){
  Message* msg;

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

Message* createFakeMessage() {
  Message* msg;

  if (!(msg = malloc(sizeof(Message))))
    return NULL;

  msg->marker = INIT_MARKER;
  msg->size = 6;
  msg->sequence = 1;
  msg->type = 2;
  memcpy(msg->data, "Hello", 6);
  msg->error = calculateCRC8("Hello", 6);

  return msg;
}

/*
 * Deletes a fully acknowledged message
 */
void deleteMessage(Message* msg, unsigned int sizeAck){
  if (sizeAck == msg->size) free(msg);
}

/*
 * The process of sending a 
 * message from the server to the user
 */
unsigned int sendMessage(int sockfd, Message* msg){
  if (msg == NULL) return -1;
  
  size_t messageSize = sizeof(Message) - MAX_DATA_SIZE + msg->size;
  uint8_t buffer[messageSize];
  memcpy(buffer, msg, messageSize);

  ssize_t sentBytes = rawSocketSend(sockfd, buffer, messageSize, 0);

  return (sentBytes == messageSize) ? 0 : -1;
}

/*
 * The process of receiving a message from the user
 * to the server
 */
Message* receiveMessage(int sockfd){
  uint8_t buffer[sizeof(Message)];
  ssize_t receivedBytes = recv(sockfd, buffer, sizeof(buffer), 0);

  Message* msg = (Message*)malloc(sizeof(Message));
  if (receivedBytes <= 0) return NULL;

  memcpy(msg, buffer, receivedBytes);
  if (msg->marker != INIT_MARKER) return NULL;
  return msg; 
}

/*
 * Calculates CRC-8 (code is being used as example)
 */
uint8_t calculateCRC8(const uint8_t *data, uint8_t len) {
  uint8_t crc = 0x00;
  for (size_t i = 0; i < len; i++) {
    crc ^= data[i];
    for (uint8_t j = 0; j < 8; j++) {
      if (crc & 0x80) {
        // Example
        crc = (crc << 1) ^ 0x07; 
      } else {
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
void ackHandler(Message* msg){
  if (msg == NULL){
    printf("NULL message received.\n");
    return;
  }
  printf("Ack received to message ID: %d\n", msg->sequence);

  deleteMessage(msg, msg->size);

  Message* nextMsg = dequeueMessage();

  if (nextMsg != NULL){
    int sockfd = 0;

    if(sendMessage(sockfd, nextMsg) == 0){
      printf("Next message sent: ID: %d\n", nextMsg->sequence);
    }
  }
}

/*
 * When an NACK is received the frame
 * in which the NACK was found is re-sent
 */
void nackHandler(Message* msg){
  if (msg == NULL){
    printf("NULL message received.\n");
    return;
  }

  printf("NACK received to message ID: %d\n", msg->sequence);
  
  msg = dequeueMessage();

  if(msg != NULL){
    int sockfd = 0;

    if(sendMessage(sockfd, msg) == 0){
      printf("Message re-sent: ID: %d\n", msg->sequence);
    }
  }
}

/*
 * When an LIST is received the
 * media array is displayed in the UI
 * 
 */
void listHandler(Message* msg){

}

/*
 * When a DOWNLOAD is received 
 */
void downloadHandler(Message* msg){

}

/*
 * when a SHOW is received 
 *    
 */
void showHandler(Message* msg){

}

/*
 *
 */
void fileInfoHandler(Message* msg){

}

/*
 *
 */
void dataHandler(Message* msg){

}

/*
 *
 */
void endHandler(Message* msg){
    
}

/*
 *
 */
void errorHandler(Message* msg){
  switch(msg->error){
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
void answerHandler(Message* msg){
  switch(msg->type){
    case ACK:
      ackHandler(msg); 

      break;
    case NACK:
      nackHandler(msg); 

      break;
    case LIST:
      listHandler(msg); 
     
      break;
    case DOWNLOAD:
      downloadHandler(msg); 
     
      break;
    case SHOW:
      showHandler(msg); 
     
      break;
    case FILE_INFO:
      fileInfoHandler(msg); 
     
      break;
    case DATA:
      dataHandler(msg); 
     
      break;
    case END:
      endHandler(msg); 
     
      break;
  case ERROR:
      errorHandler(msg);

      break;
    default:
      printf("invalid message type: %u\n", msg->type);
      break;
  }
}
