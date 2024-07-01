#ifndef __MESSAGE__
#define __MESSAGE__

#include <stdint.h>

#define MAX_DATA_SIZE 63

/*
 * frame elements unsigned
 */
typedef enum {
  ACK = 0x00,
  NACK = 0x01,
  LIST = 0x0A,
  DOWNLOAD = 0x0B,
  SHOW = 0x10,
  FILE_INFO = 0x11,
  DATA = 0x12,
  END = 0x1E,
  ERROR = 0x1F
} MessageType;

typedef enum {
  ACCESS_DENIED = 1,
  NOT_FOUND = 2,
  DISK_IS_FULL = 3
} errorType;

typedef struct message {
  uint8_t marker;
  uint8_t size: 6;
  uint8_t sequence: 5;
  uint8_t type: 5;
  uint8_t data[MAX_DATA_SIZE];
  uint8_t error;
} message;

typedef struct messageQueue {
  message* message;
  struct messageQueue* next;

} messageQueue;

/*
 * Creates a message
 */
message* createMessage(uint8_t size, uint8_t sequence, uint8_t type, uint8_t* data, uint8_t error);

/*
 * CRC-8
 */
uint8_t calculateCRC8(const uint8_t *data, uint8_t len);

/*
 * Deletes a fully acknowledged message
 */
void deleteMessage(message* msg, unsigned int sizeAck);

/*
 * The process of sending a 
 * message from the server to the user
 */
unsigned int sendMessage(int sockfd, message* msg);

/*
 * The process of recieving a message from the user
 * to the server
 */
message* receiveMessage(int sockfd);

void ackHandler(message* msg);
void nackHandler(message* msg);
void listHandler(message* msg);
void downloadHandler(message* msg);
void showHandler(message* msg);
void fileInfoHandler(message* msg);
void dataHandler(message* msg);
void endHandler(message* msg);
void errorHandler(message* msg);

/*
 * Puts the messages type to a switchCase that
 * will direct the message into its own functions
 * handler
 */
void answerHandler(message* msg);

#endif
