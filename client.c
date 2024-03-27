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
  int nread;
  char text[BUF_SIZE]; //variavel onde se vai guardar o que o cliente quer enviar para o server

  

  while(1){
    fgets(text,BUF_SIZE,stdin);  // recebe o texto do teclado
    if(strcmp(text,"SAIR\n") == 0){ // verifica se o que foi escrito é SAIR;
      write(fd, text, 1 + strlen(text)); //Escreve SAIR
      //nread = read(fd, buffer,BUF_SIZE -1); 
      //buffer[nread] = '\0';
      //printf("%s\n",buffer); //le a mensagem do servidor e depois encerra a sessão
      break;
    }
    write(fd, text, 1 + strlen(text));
    //nread = read(fd,buffer,BUF_SIZE-1); // mensagem (server) que diz se a mensagem enviada corresponde a um dos endereços do ficheiro de texto
    //buffer[nread] = '\0';
    //printf("%s\n",buffer);
  }
  close(fd);
  exit(0);
}

void erro(char *msg) {
  printf("Erro: %s\n", msg);
	exit(-1);
}