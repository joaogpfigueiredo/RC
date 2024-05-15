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
#include <pthread.h>
#include <signal.h>

#define BUF_SIZE 1024

pthread_t close_thread, client_thread;
int fd, running = 1;

void erro(char *msg) {
  printf("Erro: %s\n", msg);
	exit(-1);
}

void cleanUp() {
    running = 0;

    pthread_cancel(client_thread);
    pthread_cancel(close_thread);

    close(fd);
}

void signalHandler(int signum) {
  if(signum == SIGINT) {
    printf("CTRL+C pressed\n");
    
    cleanUp();

    exit(0);
  }
}

void *clientThread(void *arg) {
  int fd = *(int *) arg;

  printf("Client thread\n");
  
  int nread = 0;
  char buffer[BUF_SIZE], text[BUF_SIZE];

  nread = read(fd, buffer, BUF_SIZE -1); // Le a mensagem username
  buffer[nread] = '\0';
  printf("%s\n", buffer);

  fgets(text, BUF_SIZE, stdin); // Introduzir user
  write(fd, text, 1 + strlen(text));

  nread = read(fd, buffer, BUF_SIZE - 1); // Lê se vem ok
  buffer[nread] = '\0';

  if(strcmp(buffer, "OK") == 0) {
    printf("%s\n", buffer);

    nread = read(fd, buffer, BUF_SIZE - 1); // Lê a mensagem username
    buffer[nread] = '\0';
    printf("%s\n", buffer);
    
    while(running) {
      fgets(text, BUF_SIZE, stdin);
      write(fd, text, 1 + strlen(text));
      
      nread = read(fd, buffer, BUF_SIZE - 1);
      buffer[nread] = '\0';
      
      printf("%s\n",buffer);
      fflush(stdout);
    }

  } else{
    printf("%s\n",buffer);
  }

  close(fd);
  pthread_exit(NULL);
}

void *serverCloseThread(void *arg) {
  int fd = *(int *) arg;

  int nread = 0;
  char buffer[BUF_SIZE];

  while(running) {
    nread = read(fd, buffer, BUF_SIZE - 1);
    buffer[nread] = '\0';

    if(strcmp(buffer, "CLOSE") == 0) {
      printf("Server is closing\n");
      
      cleanUp();

      exit(0);
    }
  }

  pthread_exit(NULL);
}

int main(int argc, char *argv[]) {

  if (argc != 3) {
    printf("cliente <host> <port>\n");
    exit(-1);
  }

  signal(SIGINT, signalHandler);

  char endServer[100];
  struct sockaddr_in addr;
  struct hostent *hostPtr;

  strcpy(endServer, argv[1]);
  if ((hostPtr = gethostbyname(endServer)) == 0)
    erro("Não consegui obter endereço");

  bzero((void *) &addr, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = ((struct in_addr *)(hostPtr->h_addr))->s_addr;
  addr.sin_port = htons((short) atoi(argv[2]));

  if ((fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) erro("socket");

  if (connect(fd, (struct sockaddr *)&addr,sizeof (addr)) < 0) erro("Connect");

  if(pthread_create(&client_thread, NULL, clientThread, (void *)&fd) != 0){
        perror("Error creating UDP thread");
        exit(EXIT_FAILURE);
  }

  if(pthread_create(&close_thread, NULL, serverCloseThread, (void *)&fd) != 0){
        perror("Error creating UDP thread");
        exit(EXIT_FAILURE);
  }

  if(pthread_join(client_thread, NULL) != 0){
        perror("Error joining UDP thread");
        exit(EXIT_FAILURE);
  }
    
  if(pthread_join(close_thread, NULL) != 0){
        perror("Error joining UDP thread");
        exit(EXIT_FAILURE);
  }

  return 0;
}