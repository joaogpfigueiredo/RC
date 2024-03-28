/**********************************************************************
 * CLIENTE liga ao servidor (definido em argv[1]) no porto especificado
 * (em argv[2]), escrevendo a palavra predefinida (em argv[3]).
 * USO: >cliente <enderecoServidor>  <porto>  <Palavra>
 **********************************************************************/
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>

#define BUF_SIZE 1024

void erro(char *msg);

int main(int argc, char *argv[]) {
  char endServer[100];
  int fd;
  struct sockaddr_in addr;
  struct hostent *hostPtr;

  if (argc != 3) {
    printf("cliente <host> <port>\n");
    exit(-1);
  }

  strcpy(endServer, argv[1]);
  if ((hostPtr = gethostbyname(endServer)) == 0)
    erro("Não consegui obter endereço");

  bzero((void *) &addr, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = ((struct in_addr *)(hostPtr->h_addr))->s_addr;
  addr.sin_port = htons((short) atoi(argv[2]));

  if ((fd = socket(AF_INET,SOCK_STREAM,0)) == -1)
	  erro("socket");
  if (connect(fd,(struct sockaddr *)&addr,sizeof (addr)) < 0)
	  erro("Connect");

  char buffer[BUF_SIZE];
  int nread = 0;
  char text[BUF_SIZE]; //variavel onde se vai guardar o que o cliente quer enviar para o server
  char role [BUF_SIZE];

  nread = read(fd, buffer, BUF_SIZE -1); //le a mensagem username
  buffer[nread] = '\0';
  printf("%s", buffer);

  fgets(text, BUF_SIZE, stdin); //introduzir user
  write(fd, text, 1 + strlen(text));

  nread = read(fd, buffer, BUF_SIZE -1); //le a mensagem password
  buffer[nread] = '\0';
  printf("%s", buffer);

  fgets(text, BUF_SIZE, stdin);
  write(fd, text, 1 + strlen(text));

  nread = read(fd, buffer, BUF_SIZE - 1); //le se vem ok
  buffer[nread] = '\0';
  printf("%s\n", buffer);

  if(strcmp(buffer,"OK") == 0){
    while(1){
      // Código a fazer
    }
  }
  close(fd);
  exit(0);
}

void erro(char *msg) {
  printf("Erro: %s\n", msg);
	exit(-1);
}