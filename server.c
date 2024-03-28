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
#define USER_LENGTH 16
#define ROLE_LENGTH 16
#define PASSWORD_LENGTH 20
#define MAX_LINE_LENGTH 300

int login(const char *filename, const char *username, const char *password, int client_fd);

void *process_client(void *arg) {
    int client_socket = *((int *)arg);
    free(arg);

    int nread;
    char buffer[BUF_SIZE];
    char mensagem[BUF_SIZE];
    char username[BUF_SIZE];
    char password[BUF_SIZE];

    write(client_socket,"Bem-Vindo ao Servior!\nDigite o seu username: ", 1 + strlen("Bem-Vindo ao Servior!\nDigite o seu username: "));
    nread = read(client_socket, username, BUF_SIZE-1);
    username[nread] = '\0';
    username[strcspn(username, "\r\n")] = 0;

    write(client_socket,"Digite a sua password: ", strlen("Digite a sua password: "));
    nread = read(client_socket, password, BUF_SIZE-1);
    password[nread] = '\0';
    password[strcspn(password, "\r\n")] = 0;

    if(login("ficheiro_config.txt", username, password, client_socket) == 1){
        do{
            nread = read(client_socket, buffer, BUF_SIZE-1);
            buffer[nread] = '\0';
            buffer[strcspn(buffer, "\r\n")] = 0;
            if(strcmp(buffer,"SAIR") == 0){
                write(client_socket, "SAIR", 1 + strlen("SAIR"));
                break;
            }

            sprintf(mensagem, "Comando recebido com sucesso.");
            write(client_socket, mensagem, 1 + strlen(mensagem));

            fflush(stdout);
        }while(nread > 0);
    }
    close(client_socket); 
    return NULL;
}

void *verifica_tcp(void *arg){ //void *arg permite que sejam passados qualquer tipo de variavel
    int tcp_fd = *((int *) arg); //Vai buscar o valor apontado pelo ponteiro arg (Em int)
    struct sockaddr_in client_addr;
    int client, client_addr_size;
    //client_addr_size = sizeof(client_addr);
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

//void *verifica_udp(void *arg){

//}

int login(const char *filename, const char *username, const char *password, int client_fd) {
    FILE *file;
    char line[MAX_LINE_LENGTH];
    char fusername [MAX_LINE_LENGTH];
    char fpassword [MAX_LINE_LENGTH];
    char frole [MAX_LINE_LENGTH];
    char mensagem[BUF_SIZE];

    // Tenta abrir o arquivo para leitura
    file = fopen(filename, "r");
    if (file == NULL) {
        fprintf(stderr, "Não foi possível abrir o arquivo %s\n", filename);
        return 0;
    }

    // Lê o arquivo linha por linha
    while (fgets(line, MAX_LINE_LENGTH, file)){
        if (sscanf(line, "%[^;];%[^;];%[^;]", fusername, fpassword, frole) == 3) {
            if (strcmp(username, fusername) == 0 && strcmp(password, fpassword) == 0) {
                write(client_fd, frole, 1 + strlen(frole));
                fclose(file);
                return 1;
            }
        }
    }
    sprintf(mensagem, "REJECTED");
    write(client_fd, mensagem, 1 + strlen(mensagem));
    fclose(file);
    return 0;
}

void erro(char *msg){
	printf("Erro: %s\n", msg);
	exit(-1);
}

int main(){
    int tcp_fd; 
    int udp_fd;
    struct sockaddr_in tcp_addr;
    struct udp_addr;
    pthread_t thread_tcp; 
    pthread_t thread_udp;

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