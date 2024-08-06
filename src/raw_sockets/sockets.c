#include <stdio.h>
#include <stdlib.h> 
#include <arpa/inet.h> 
#include <unistd.h>
#include <errno.h>

#include <net/ethernet.h> 
#include <linux/if_packet.h> 
#include <sys/types.h>
#include <sys/socket.h>
#include <stddef.h> 
#include <net/if.h> 
#include <unistd.h>
#include "sockets.h"

int rawSocketCreator(char* network_interface_name) { 

  // Cria arquivo para o socket sem qualquer protocolo 

      //sockfd         //domain    //type    //protocol
  int rsocket = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL)); 

  if (rsocket == -1) { 
    fprintf(stderr, "Erro ao criar socket: Verifique se você é root!\n"); exit(-1); 
  }

  //converte a interface de rede em um index 
  int ifindex = if_nametoindex(network_interface_name);
 
  //inicializa a struck com zeros
  struct sockaddr_ll endereco = {0};

  //define a familia de enderecos
  endereco.sll_family = AF_PACKET;

  //define o protocolo em para todos os pacotes Ethernet
  endereco.sll_protocol = htons(ETH_P_ALL);
  
  //define a interface de rede pelo index convertido
  endereco.sll_ifindex = ifindex;

  //associa o socket ao endereço especificado
  if (bind(rsocket, (struct sockaddr*) &endereco, sizeof(endereco)) == -1) {
    fprintf(stderr, "Erro ao fazer bind no socket\n");
    exit(-1);
  }

  //inicializa a struck com zeros
  struct packet_mreq mr = {0};

  //define o indice da interface
  mr.mr_ifindex = ifindex;

  //define o tipo como modo promíscuo(Captura todos os pacotes que passam pela interface)
  mr.mr_type = PACKET_MR_PROMISC;

  // Não joga fora o que identifica como lixo: Modo promíscuo
  if (setsockopt(rsocket, SOL_PACKET, PACKET_ADD_MEMBERSHIP, &mr, sizeof(mr)) == -1) {
    fprintf(stderr, "Erro ao fazer setsockopt: ""Verifique se a interface de rede foi especificada corretamente.\n");
    exit(-1);
  }

  return rsocket;
}

int rawSocketSend(int rsocket, const void *buffer, unsigned int length, int flags){
  return (int) write(rsocket, buffer, length);
}
