#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include "../raw_sockets/sockets.h"

int main()
{
  unsigned int rsocket = rawSocketCreator("lo");

  char last_received[64000];
  int last_received_size = 0;

  while (1)
  {
    char buffer[64000];
    ssize_t data_size = recv(rsocket, buffer, 64000, 0);
    if (data_size == -1)
    {
      perror("Erro ao receber dados");
      exit(-1);
    }

    if (data_size == last_received_size && memcmp(buffer, last_received, data_size) == 0)
    {
      printf("Ignorando pacote duplicado\n");
      continue;
    }

    // Update last received packet information
    memcpy(last_received, buffer, data_size);
    last_received_size = data_size;

    printf("Recebido %zd bytes\n", data_size);

    // Handle received data as needed
    // Example: Print the received data
    printf("Dados recebidos: %.*s\n", (int)data_size, buffer);
  }

  close(rsocket);
  return 0;
}