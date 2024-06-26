# Projeto de Comunicação em Rede via Raw Sockets

## Descrição

Este projeto implementa um protocolo de comunicação em rede usando raw sockets para comunicação direta entre duas máquinas conectadas por um cabo de rede. O sistema desenvolvido consiste em dois elementos, um cliente e um servidor, que permitem a transferência de arquivos de vídeo. O protocolo é inspirado no Kermit e usa uma janela deslizante com estratégia Go-Back-N para controle de fluxo.

## Formato do Frame

Os frames de comunicação seguem a estrutura abaixo:

| Campo                   | Tamanho  | Descrição                                                                                 |
|-------------------------|----------|-------------------------------------------------------------------------------------------|
| **Marcador de Início**  | 8 bits   | Indica o começo de uma nova mensagem.                                                     |
| **Tamanho da Mensagem** | 6 bits   | Tamanho da mensagem em bytes, utilizado para verificação de entrega e para indicar o tamanho dos dados. |
| **Sequência da Mensagem** | 5 bits  | Ordem de interpretação dos pacotes.                                                       |
| **Tipo da Mensagem**    | 5 bits   | Define o tipo da mensagem. Veja os valores na seção abaixo.                               |
| **Dados**               | 0-63 bytes | Conteúdo da mensagem, especificado no campo Tamanho da Mensagem.                         |
| **Detecção de Erros**   | 8 bits   | CRC-8 para detecção de erros.                                                             |

### Tipos de Mensagem

Os tipos de mensagem são definidos como:

- `00000` ack
- `00001` nack
- `01010` lista
- `01011` baixar
- `10000` mostra na tela
- `10001` descritor arquivo
- `10010` dados
- `11110` fim tx
- `11111` erro

### Erros Reportados

Os erros que podem ser reportados são:

1. Acesso negado
2. Não encontrado
3. Disco cheio

## Controle de Fluxo

O controle de fluxo é implementado usando a técnica de janela deslizante Go-Back-N com uma janela fixa de 5 mensagens para transmissões de arquivos e uma mensagem para outros tipos de comunicação.

## Implementação

A implementação deve seguir as diretrizes abaixo:

- **Linguagem:** C ou C++ devido ao uso de raw sockets.
- **Ambiente:** As máquinas devem operar em modo root e estar interligadas diretamente por cabo de rede.
- **Funções do Cliente e Servidor:**
  - **Servidor:** Lista e envia vídeos disponíveis.
  - **Cliente:** Exibe a lista de vídeos, permite a seleção e solicita a transferência, chamando um player para exibição após a transferência.
- **Timeouts e Tratamento de Erros:** Deve-se implementar a gestão de timeouts e tratamento de possíveis erros na comunicação.

## Desenvolvimento e Entrega

O trabalho deve ser feito em duplas. A entrega deve incluir:

1. Relatório de uma ou duas páginas no formato de artigo, incluindo nomes dos componentes e link para o repositório Git.
2. Código fonte compactado em `tar.gz`.
3. Outros arquivos de teste, se necessário.

A apresentação será realizada em laboratório, e todos os membros do grupo devem estar presentes e dominar totalmente o trabalho.

## Registro de Aulas e Material de Apoio

Material de apoio será disponibilizado no UFPR Virtual da disciplina. Fotos do quadro durante as aulas também estão disponíveis para referência.

