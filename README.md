fazer comunicacao simples raw socket
    Realizar testes de comunicação entre dois computadores
    Utilizando cabo de rede 



QUEBRANDO A DISTRIBUIÇÃO DE BITS -> MENSAGEM <-

-> Marcador de Início <- 8 bits
    Demarca o começo de uma nova mensagem

-> Tamanho da Mensagem <- 6 bits
    Tamanho da mensagem em bytes 
    Deverá ser utilizado para 
    verificação de entrega 
    da mensagem. 

    Também será utilizado para informar o tamanho do DADOS 

-> SEQUENCIA DA MENSAGEM <- 5 bits
    Ordem de interpretação 
    dos pacotes

-> TIPO DA MENSAGEM <- 5 bits
    Os tipos de mensagens (campo “Tipo”) são:
    00000 ack
    00001 nack
    01010 lista
    01011 baixar
    10000 mostra na tela
    10001 descritor arquivo
    10010 dados
    11110 fim tx
    11111 erro

-> DADOS <- X bytes (0 a 63 bytes)
    Será informado no campo “Tamanho da Mensagem” o tamanho da mensagem

-> Detecção de ERROS <- 8 bits
    Algoritimo CRC-8

    Os erros reportados podem ser:
    1 acesso negado
    2 não encontrado
    3 disco cheio

fazer sliding window go back  no protocolo listado acima para comunicação 
entre as máquinas, para que não haja perda de pacotes no processo


implementar de fato (listar, baixar, etc)
