#include <stdint.h>

typedef struct message {
  uint8_t marker;
  uint8_t size: 6;
  uint8_t sequence: 5;
  uint8_t type: 5;
  uint8_t *data;
  uint8_t error;
  
} message;

/*
 * Creates a message
 */
message* createMessage(uint8_t marker, uint8_t size, uint8_t sequence, uint8_t type, uint8_t* data, uint8_t error);

/*
 * memory allocs the necessary space for the data array
 */
uint8_t* mallocData();

/*
 * Deletes a fully acknowledged message
 */
void deleteMessage(message* msg, unsigned int sizeAck);

/*
 * The process of sending a 
 * message from the server to the user
 */
unsigned int sendMessage();


