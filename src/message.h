
typedef struct message {
  unsigned int marker;
  unsigned int size;
  unsigned int sequence;
  unsigned int type;
  unsigned int* data;
  unsigned int error;
  
} message;
