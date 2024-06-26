#include <stdint.h>

#define MAX_DATA_SIZE 63

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


typedef struct message {
  uint8_t marker;
  uint8_t size: 6;
  uint8_t sequence: 5;
  uint8_t type: 5;
  uint8_t data[MAX_DATA_SIZE];
  uint8_t error;
} message;

/*
 * Creates a message
 */
message* createMessage(uint8_t marker, uint8_t size, uint8_t sequence, uint8_t type, uint8_t* data, uint8_t error);

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
