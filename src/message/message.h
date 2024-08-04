#ifndef __MESSAGE__
#define __MESSAGE__

#include <stdint.h>

#define MAX_DATA_SIZE 63
#define MAX_SEQUENCE 32
#define WINDOW_SIZE 5
#define INIT_MARKER 0x7E

/*
 * frame elements unsigned
 */
typedef enum
{
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

typedef enum
{
  ACCESS_DENIED = 1,
  NOT_FOUND = 2,
  DISK_IS_FULL = 3
} ErrorType;

typedef struct message
{
  uint8_t marker;
  uint8_t size : 6;
  uint8_t sequence : 5;
  uint8_t type : 5;
  uint8_t data[MAX_DATA_SIZE];
  uint8_t error;
} Message;

long long timestamp();
Message *createMessage(uint8_t size, uint8_t sequence, uint8_t type, uint8_t data[]);
int sendMessage(int sockfd, Message *msg);
Message *receiveMessage(int sockfd);
uint8_t calculateCRC8(const uint8_t *data, uint8_t len);
void listHandler(int sockfd);
void downloadHandler(Message *receivedBytes, int sockfd);
void answerHandler(Message *msg, int sockfd);

#endif
