#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "message.h"

#define MAX_DATA_SIZE 63

#define ACK 00000
#define NACK 00001
#define LIST 01010
#define DOWNLOAD 01011
#define SHOW 10000
#define FILE_INFO 10001
#define DATA 10010
#define END 11110
#define ERROR 11111

/*
 * Creates a message
 */
message* createMessage(uint8_t marker, uint8_t size, uint8_t sequence, uint8_t type, uint8_t data[], uint8_t error){
  message* msg;

  if (!(msg = malloc(sizeof(message))))
    return NULL;

  msg->marker = marker;
  msg->size = size;
  msg->sequence = sequence;
  msg->type = type;
  msg->data = mallocData();
  msg->error = error;

  return msg;
}

uint8_t* mallocData(){
  uint8_t* data;
  
  if ((data = malloc(sizeof(uint8_t) * MAX_DATA_SIZE))) return data;

  return NULL;
}

/*
 * Deletes a fully acknowledged message
 */
void deleteMessage(message* msg, unsigned int sizeAck){
  if (sizeAck == msg->size) free(msg);
}

/*
 * The process of sending a 
 * message from the server to the user
 */
unsigned int sendMessage(){

  return NULL;
}


