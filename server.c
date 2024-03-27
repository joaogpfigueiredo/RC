#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <netdb.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <pthread.h>

#define PORTO_TURMAS 9000
#define PORTO_CONFIG 9876
#define BUF_SIZE 1024
#define MAX_LINE_LENGTH 300

volatile int count = 0;

void *process_client(void *arg) {
    int client_socket = *((int *)arg);
    free(arg);

    int nread;
    char buffer[BUF_SIZE];

    count ++;
    printf("Ligado, numero de ligações: %d\n",count);

    do{
        nread = read(client_socket , buffer, BUF_SIZE-1);
        buffer[nread] = '\0';
        buffer[strcspn(buffer, "\r\n")] = 0;
        printf("Cliente disse: %s\n",buffer);
        if (strcmp(buffer,"SAIR")==0){
            count--;
            printf("Adeus, numero de ligações: %d\n",count);
            break;
        }
        fflush(stdout);
    }while(nread > 0);

    close(client_socket); 
    return NULL;
}

void *verifica_tcp(void *arg){ //void *arg permite que sejam passados qualquer tipo de variavel
    int tcp_fd = *((int *) arg); //Vai buscar o valor apontado pelo ponteiro arg (Em int)
    struct sockaddr_in client_addr;
    int client , client_addr_size;
    client_addr_size = sizeof(client_addr);
    
    while(1) {
        struct sockaddr_in client_addr;
        socklen_t client_addr_size = sizeof(client_addr);
        int *client = malloc(sizeof(int));
        if ((*client = accept(tcp_fd, (struct sockaddr *)&client_addr, &client_addr_size)) < 0) {
            perror("Error in accept");
            free(client); // Libertar a memória se o accept falhar
            continue;
        }
        pthread_t client_thread;
        if(pthread_create(&client_thread, NULL, process_client, client) != 0) {
            perror("Failed to create thread for client");
            close(*client);
            free(client); // Libertar a memória se a criação da thread falhar
        }
        pthread_detach(client_thread); // Isto permite que a thread liberte recursos quando terminar
    }
}

void *verifica_udp(void *arg){

}

void load_users(const char *filename, userList list) {
    FILE *file;
    char line[MAX_LINE_LENGTH];
    struct User user;

    // Tenta abrir o arquivo para leitura
    file = fopen(filename, "r");
    if (file == NULL) {
        fprintf(stderr, "Não foi possível abrir o arquivo %s\n", filename);
        return;
    }

    // Lê o arquivo linha por linha
    while (fgets(line, MAX_LINE_LENGTH, file) != NULL) {
        if (sscanf(line, "%s;%s;%s", user.username, user.password, user.role) == 3) {
            insere(list, user);
        }
    }

    fclose(file);
}


void erro(char *msg){
	printf("Erro: %s\n", msg);
	exit(-1);
}

int main(){

    userList Userlist;
    Userlist = create();

    setbuf(stdout, 0);

    if(Userlist == NULL){
        printf("Allocation Error.\n");
        return -1;
    }

    Userlist->prox = NULL;

    int tcp_fd, udp_fd;
    struct sockaddr_in tcp_addr, udp_addr;
    pthread_t thread_tcp, thread_udp;

    //////////////////////////////////TCP//////////////////////////////////////////

    bzero((void *) &tcp_addr, sizeof(tcp_addr));
    tcp_addr.sin_family = AF_INET;
    tcp_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    tcp_addr.sin_port = htons(PORTO_TURMAS);
 
    if ( (tcp_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	erro("Ao criar a socket TCP");

    if ( bind(tcp_fd,(struct sockaddr*)&tcp_addr,sizeof(tcp_addr)) < 0)
	erro("na funcao bind");

    ///////////////////////////////////////////////////////////////////////////////
    
    //////////////////////////////////UDP//////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////

    if (pthread_create(&thread_tcp, NULL, verifica_tcp, (void *)&tcp_fd) != 0) {
        perror("Error creating TCP thread");
        exit(EXIT_FAILURE);
    }  
    if( listen(tcp_fd, 5) < 0) // numero de ligacoes em simultaneo suportadas
	erro("na funcao listen");

    pthread_join(thread_tcp, NULL);

    return 0;
}