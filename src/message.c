#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/socket.h>
#include "message.h"

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
  memcpy(msg->data, data, size);
  msg->error = calculateCRC8(data, size);

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
 * Deletes a fully acknowledged message
 */
void deleteMessage(message* msg, unsigned int sizeAck){
  if (sizeAck == msg->size) free(msg);
}

/*
 * The process of sending a 
 * message from the server to the user
 */
unsigned int sendMessage(int sockfd, message* msg){
  if (msg == NULL) return -1;
  
  size_t messageSize = sizeof(message) - MAX_DATA_SIZE + msg->size;
  uint8_t buffer[messageSize];
  memcpy(buffer, msg, messageSize);

  ssize_t sentBytes = send(sockfd, buffer, messageSize, 0);

  return (sentBytes == messageSize) ? 0 : -1;
}

/*
 * The process of recieving a message from the user
 * to the server
 */
message* receiveMessage(int sockfd){
  uint8_t buffer[sizeof(message)];
  ssize_t receivedBytes = recv(sockfd, buffer, sizeof(buffer), 0);

  message* msg = (message*)malloc(sizeof(message));
  if (receivedBytes <= 0) return NULL;

  memcpy(msg, buffer, receivedBytes);
 return msg; 
}
