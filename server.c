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

#define PORTO_TURMAS 9003
#define PORTO_CONFIG 9876
#define BUF_SIZE 1024
#define USER_LENGTH 16
#define ROLE_LENGTH 16
#define PASSWORD_LENGTH 20
#define MAX_LINE_LENGTH 300
#define opc_alunos "Lista de comandos para Alunos:\n> LIST_CLASSES\n> LIST_SUBSCRIBED\n> SUBSCRIBE_CLASS <nome>\n> CTRL+C para encerrar"
#define opc_professores "Lista de comandos para Professores:\n> LIST_CLASSES\n> LIST_SUBSCRIBED\n> CREATE_CLASS <nome> <size>\n> SEND <nome> <mensagem>\n> CTRL+C para encerrar"

int login(const char *filename, const char *username, const char *password, int client_fd, char *role);

void *process_client(void *arg) {
    int client_socket = *((int *)arg);
    free(arg);

    int nread;
    char buffer[BUF_SIZE], mensagem[BUF_SIZE], comando[BUF_SIZE], username[BUF_SIZE], 
    password[BUF_SIZE], role[BUF_SIZE], args[BUF_SIZE], args1[BUF_SIZE];

    write(client_socket,"Bem-Vindo ao Servior! (LOGIN <username> <password>): ", 1 + strlen("Bem-Vindo ao Servior! (LOGIN <username> <password>): "));
    nread = read(client_socket, buffer, BUF_SIZE-1);
    buffer[nread] = '\0';
    buffer[strcspn(buffer, "\r\n")] = 0;

    if(sscanf(buffer,"%s %s %s",comando,username,password)==3){
        if(strcmp(comando,"LOGIN") == 0){
            if(login("ficheiro_config.txt", username, password, client_socket, role) == 1){
                role[strcspn(role, "\r\n")] = 0;
                if(strcmp(role,"aluno") == 0){
                    write(client_socket, opc_alunos, 1 + strlen(opc_alunos));
                } else if(strcmp(role,"professor") == 0){
                    write(client_socket,opc_professores, 1 + strlen(opc_professores));
                }
                do{
                    nread = read(client_socket, buffer, BUF_SIZE-1);
                    buffer[nread] = '\0';
                    buffer[strcspn(buffer, "\r\n")] = 0;
                    if(strcmp(role,"aluno") == 0){
                        if(sscanf(buffer,"%s %s",comando,args) != 2){
                            if(strcmp(buffer,"LIST_CLASSES") == 0 || strcmp(buffer,"LIST_SUBSCRIBED") == 0){
                                sprintf(mensagem, "Comando recebido com sucesso!");
                                write(client_socket, mensagem, 1 + strlen(mensagem));
                                break;                                    
                            } else{
                                write(client_socket,"Comando Inválido!",strlen("Comando Inválido\n"));
                            }
                        } else{
                            if(strcmp(comando, "SUBSCRIBE_CLASS") == 0){
                                sprintf(mensagem, "Comando recebido com sucesso!");
                                write(client_socket, mensagem, 1 + strlen(mensagem));
                                break;                                    
                            } else{
                                write(client_socket,"Comando Inválido!",strlen("Comando Inválido\n"));
                            }                            
                        }
                        
                    } else{
                        if(sscanf(buffer,"%s %s %s",comando,args,args1) != 3){
                            if(strcmp(buffer,"LIST_CLASSES") == 0 || strcmp(buffer,"LIST_SUBSCRIBED") == 0){
                                sprintf(mensagem, "Comando recebido com sucesso!");
                                write(client_socket, mensagem, 1 + strlen(mensagem));
                                break;                                    
                            } else{
                                write(client_socket,"Comando Inválido!",strlen("Comando Inválido\n"));
                            }
                        } else{
                            if(strcmp(comando,"CREATE_CLASS") == 0 || strcmp(comando,"SEND") == 0){
                                sprintf(mensagem, "Comando recebido com sucesso!");
                                write(client_socket, mensagem, 1 + strlen(mensagem));
                                break;                                    
                            } else{
                                write(client_socket,"Comando Inválido!",strlen("Comando Inválido\n"));
                            }
                        }
                    }
                    fflush(stdout);
                }while(nread > 0);
            }
        } else{
            write(client_socket,"Comando Inválido!",strlen("Comando Inválido!"));
        }
    }else{
        write(client_socket,"Comando Inválido ou Numero de Argumentos Inválido!",strlen("Comando Inválido ou Numero de Argumentos Inválido!"));
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

void *process_admin(void *arg){
    int udp_fd = *((int *)arg);
    free(arg);

    struct sockaddr_in si_minha, si_outra;
    int recv_len;
	socklen_t slen = sizeof(si_outra);
    char buf[BUF_SIZE];

	// Cria um socket para recepção de pacotes UDP
	if((udp_fd=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
		perror("Erro na criação do socket");
	}

    // Preenchimento da socket address structure
	si_minha.sin_family = AF_INET;
	si_minha.sin_port = htons(PORTO_CONFIG);
	si_minha.sin_addr.s_addr = htonl(INADDR_ANY);

	// Associa o socket à informação de endereço
	if(bind(udp_fd,(struct sockaddr*)&si_minha, sizeof(si_minha)) == -1) {
		perror("Erro no bind");
	}

    while(1){
        if((recv_len = recvfrom(udp_fd, buf, BUF_SIZE, 0, (struct sockaddr *) &si_outra, (socklen_t *)&slen)) == -1) {
            perror("Erro no recvfrom");
        }
        // Para ignorar o restante conteúdo (anterior do buffer)
        buf[recv_len]='\0';
        buf[strcspn(buf, "\r\n")] = 0; // Remover o '\n' ou '\r\n' no final do buffer

        printf("Conteúdo da mensagem: %s\n" , buf);
    }
}


int login(const char *filename, const char *username, const char *password, int client_fd, char *role) {
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
                sprintf(mensagem,"OK %s",frole);
                write(client_fd,mensagem,1 + strlen(mensagem));
                strcpy(role,frole);
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

    if(pthread_create(&thread_udp, NULL, process_admin, (void *)&udp_fd) != 0){
        perror("Error creating UDP thread");
        exit(EXIT_FAILURE);
    }

    pthread_join(thread_udp, NULL);

    return 0;
}