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
#include <sys/select.h>
#include <arpa/inet.h>

#define BUF_SIZE 1024
#define MAX_TURMAS 1000

typedef struct {
  pthread_t my_treads;
  int sock;
} Turma;

Turma *turmas;

int fd, running = 1, num_turmas = -1;

void erro(char *msg) {
  printf("Erro: %s\n", msg);
	exit(-1);
}

void cleanUp() {
    running = 0;

    for(int i = 0; i < MAX_TURMAS; i++) {
      if(turmas[i].sock != 0) {
        close(turmas[i].sock);
      }

      if(turmas[i].my_treads != 0) {
        pthread_cancel(turmas[i].my_treads);
      }
    }

    free(turmas);

    close(fd);
}

void signalHandler(int signum) {
  if(signum == SIGINT) {
    printf("\nCTRL+C pressed\n");
    
    cleanUp();

    exit(0);
  }
}

void *receiveMulticastMessage(void *arg) {
  int sock = *((int *)arg);
  char buffer[BUF_SIZE];
  struct sockaddr_in localAddr;
  socklen_t addrlen = sizeof(localAddr);

  while(1) {
    int bytes_received = recvfrom(sock, buffer, BUF_SIZE, 0, (struct sockaddr *) &localAddr, &addrlen);
    if (bytes_received < 0) {
        perror("Erro ao receber a mensagem");
        exit(1);
    }
    buffer[bytes_received] = '\0';
    printf("Mensagem recebida: %s\n", buffer);
  }
}

void joinMulticast(char *buffer) {
  char *token = strtok(buffer, " ");
  token = strtok(NULL, " ");

  struct sockaddr_in localAddr;
  struct ip_mreq multicastRequest;

  // Cria um socket UDP
  if ((turmas[num_turmas].sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
      perror("Erro ao criar o socket");
      exit(1);
  }

  // Configura o endereço local para escutar as mensagens multicast
  memset(&localAddr, 0, sizeof(localAddr));
  localAddr.sin_family = AF_INET;
  localAddr.sin_addr.s_addr = htonl(INADDR_ANY);
  localAddr.sin_port = htons(8080 + num_turmas);

  
  int enable = 1;
  setsockopt(turmas[num_turmas].sock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable));

  // Associa o socket ao endereço local
  if (bind(turmas[num_turmas].sock, (struct sockaddr *) &localAddr, sizeof(localAddr)) < 0) {
      perror("Erro ao fazer o bind do socket");
      exit(1);
  }

  // Solicita a participação no grupo multicast
  multicastRequest.imr_multiaddr.s_addr = inet_addr(token);
  multicastRequest.imr_interface.s_addr = htonl(INADDR_ANY);
  if (setsockopt(turmas[num_turmas].sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, &multicastRequest, sizeof(multicastRequest)) < 0) {
      perror("Erro ao participar do grupo multicast");
      exit(1);
  }

  pthread_create(&turmas[num_turmas].my_treads, NULL, receiveMulticastMessage, (void *)&turmas[num_turmas].sock);
}

int main(int argc, char *argv[]) {

  if (argc != 3) {
    printf("cliente <host> <port>\n");
    exit(-1);
  }

  signal(SIGINT, signalHandler);

  turmas = (Turma *) malloc(sizeof(Turma) * MAX_TURMAS);

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

  fd_set read_fds;

  int nread = 0;
  int max_fd;
  char buffer[BUF_SIZE], text[BUF_SIZE];

  nread = read(fd, buffer, BUF_SIZE -1); // Le a mensagem username
  buffer[nread] = '\0';
  printf("%s\n", buffer);

  while (1) {
    FD_ZERO(&read_fds);
    FD_SET(fd, &read_fds);
    FD_SET(STDIN_FILENO, &read_fds);

    max_fd = fd > STDIN_FILENO ? fd : STDIN_FILENO;

    if (select(max_fd + 1, &read_fds, NULL, NULL, NULL) == -1) {
        perror("select");
        exit(EXIT_FAILURE);
    }

    if (FD_ISSET(fd, &read_fds)) {
      // The socket is ready to be read.
      nread = read(fd, buffer, sizeof(buffer));
      if (nread > 0) {
          buffer[nread] = '\0';

        if(strcmp(buffer, "CLOSE") == 0) {
          printf("Server is closing\n");
          
          cleanUp();

          exit(0);
        }
        memset(buffer, 0, sizeof(buffer));
      }
    }

    if (FD_ISSET(STDIN_FILENO, &read_fds)) {
      fgets(text, BUF_SIZE, stdin);
      write(fd, text, 1 + strlen(text));

      nread = read(fd, buffer, BUF_SIZE - 1);
      buffer[nread] = '\0';
      printf("%s\n", buffer);
      
      if(strcmp(buffer, "REJECTED") == 0) {
        cleanUp();
        exit(0);
      } if(strstr(buffer, "ACCEPTED") != NULL) {
        num_turmas++;
        joinMulticast(buffer);
      }

      memset(buffer, 0, sizeof(buffer));
    }
  }

  return 0;
}