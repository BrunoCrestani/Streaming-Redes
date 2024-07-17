#ifndef __MESSAGE__
#define __MESSAGE__

#include <stdint.h>

#define MAX_DATA_SIZE 63
#define INIT_MARKER 0x7E

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
} ErrorType;

typedef struct message {
  uint8_t marker;
  uint8_t size: 6;
  uint8_t sequence: 5;
  uint8_t type: 5;
  uint8_t data[MAX_DATA_SIZE];
  uint8_t error;
} Message;

typedef struct messageQueue {
  Message* message;
  struct messageQueue* next;

} messageQueue;

/*
 * Creates a message
 */
Message* createMessage(uint8_t size, uint8_t sequence, uint8_t type, uint8_t* data, uint8_t error);

Message* createFakeMessage();

/*
 * CRC-8
 */
uint8_t calculateCRC8(const uint8_t *data, uint8_t len);

/*
 * Deletes a fully acknowledged message
 */
void deleteMessage(Message* msg, unsigned int sizeAck);

/*
 * The process of sending a 
 * message from the server to the user
 */
int sendMessage(int sockfd, Message* msg);

/*
 * The process of recieving a message from the user
 * to the server
 */
Message* receiveMessage(int sockfd);

void ackHandler(Message* msg);
void nackHandler(Message* msg);
void listHandler(Message* msg);
void downloadHandler(Message* msg);
void showHandler(Message* msg);
void fileInfoHandler(Message* msg);
void dataHandler(Message* msg);
void endHandler(Message* msg);
void errorHandler(Message* msg);

/*
 * Puts the messages type to a switchCase that
 * will direct the message into its own functions
 * handler
 */
void answerHandler(Message* msg);

#endif
