#include "server.h"

void erro(char *msg){
	printf("Erro: %s\n", msg);
	exit(-1);
}

void sendMessage(int socket_client, char *buffer, struct sockaddr_in si_outra, socklen_t slen) {
    if (sendto(socket_client, buffer, strlen(buffer), 0, (struct sockaddr *)&si_outra, slen) == -1) {
        erro("Erro ao enviar mensagem UDP");
    }
}

char *createMulticastIp() {
    char *multicast_ip = malloc(sizeof(char) * 16);
    srand(time(NULL));
    sprintf(multicast_ip, "239.0.0.%d", rand() % 256);
    return multicast_ip;
}

void loadUsers() {
    FILE *file;
    char line[MAX_LINE_LENGTH];
    char fusername [MAX_LINE_LENGTH];
    char fpassword [MAX_LINE_LENGTH];
    char frole [MAX_LINE_LENGTH];
    int i = 0;

    // Tenta abrir o arquivo para leitura
    file = fopen(ficheiro_config, "r");
    if (file == NULL) {
        fprintf(stderr, "Não foi possível abrir o arquivo %s\n", ficheiro_config);
        exit(1);
    }

    // Lê o arquivo linha por linha
    while (fgets(line, MAX_LINE_LENGTH, file)){
        if (sscanf(line, "%[^;];%[^;];%[^;]", fusername, fpassword, frole) == 3) {
            strcpy(users[i].username, fusername);
            strcpy(users[i].password, fpassword);
            frole[strcspn(frole, "\r\n")] = 0;
            strcpy(users[i].role, frole);
            i++;
        }
    }
    fclose(file);
}

void saveUsers() {
    FILE *file;

    // Tenta abrir o arquivo para escrita
    file = fopen(ficheiro_config, "w");
    if (file == NULL) {
        fprintf(stderr, "Não foi possível abrir o arquivo %s\n", ficheiro_config);
        exit(1);
    }

    // Escreve os usuários no arquivo
    for(int i = 0; i < NUSERS; i++) {
        if(strcmp(users[i].username, "\0") != 0){
            fprintf(file, "%s;%s;%s\n", users[i].username, users[i].password, users[i].role);
        }
    }
    fclose(file);

}

int add_user(char *username, char *password, char* role, int udp_fd, struct sockaddr_in si_outra, socklen_t slen){
    for(int i = 0; i < NUSERS; i++) {
        if(strcmp(users[i].username,"") == 0) {
            strcpy(users[i].username, username);
            strcpy(users[i].password, password);
            strcpy(users[i].role, role);
            sendMessage(udp_fd, "Utilizador Adicionado com Sucesso!\n", si_outra, slen);
            return 1;
        }
    }
    sendMessage(udp_fd, "Problema ao Adicionar User!\n", si_outra, slen);
    return 0;
}

int del_user(const char *username, int udp_fd, struct sockaddr_in si_outra, socklen_t slen) {
    for(int i = 0; i < NUSERS; i++) {
        if(strcmp(users[i].username, username) == 0) {
            strcpy(users[i].username, "\0");
            strcpy(users[i].password, "\0");
            strcpy(users[i].role, "\0");
            sendMessage(udp_fd, "Utilizador Removido com Sucesso!\n", si_outra, slen);
            return 1;
        }
    }
    sendMessage(udp_fd, "Problema ao Remover User!\n", si_outra, slen);
    return 0;
}

void list_users(int udp_fd, struct sockaddr_in si_outra, socklen_t slen) {
    char message[BUF_SIZE], buff[BUF_SIZE];
    memset(message, 0, sizeof(message));
    for(int i = 0; i < NUSERS; i++){
        if(strcmp(users[i].username, "\0") != 0){
            sprintf(buff, "User %d: \n - Nome: %s\n - Role: %s\n", i + 1, users[i].username, users[i].role);
            strcat(message, buff);
        }
    }
    
    if(strlen(message) == 0){
        strcpy(message, "Não existem utilizadores registados!\n");
    }

    sendMessage(udp_fd, message, si_outra, slen);
    sendMessage(udp_fd, "Usuário Listados em Cima!\n", si_outra, slen);

}

int login(const char *username, const char *password, char *role) {
    
    for(int i = 0; i < NUSERS; i++) {
        if(strcmp(users[i].username, username) == 0 && strcmp(users[i].password, password) == 0) {
            strcpy(role, users[i].role);
            return 1;
        }
    }
    return 0;
}

void cleanup() {
    printf("\nA encerrar o servidor...\n");

    running = 0;

    saveUsers();

    for(int i = 0; i < userCounter; i++) {
        write(*clients[i].client, "CLOSE", strlen("CLOSE"));

        if(clients[i].client_thread != 0) pthread_cancel(clients[i].client_thread);
        
        if(close(*clients[i].client) != 0) perror("Failed to close client socket");

        free(clients[i].client);
    }

    pthread_cancel(thread_tcp);
    pthread_cancel(thread_udp);

    free(users);
    free(clients);

    close(tcp_fd);
    close(udp_fd);
}

void sign_handler(int signum) {
    if(signum == SIGINT) {
        cleanup();
    }
}

void create_class(int socket_client, char *name, char *size, char *username){
    char message[BUF_SIZE];
    memset(message, 0, sizeof(message));
    //usar um mutex para sincronizar thread ??? 
    for(int i = 0; i < MAX_TURMAS; i++){
        if(turmas[i].tamanho_max == -1){
            turmas[i].tamanho_max = atoi(size);
            turmas[i].tamanho_atual = 0;
            turmas[i].alunos = malloc(turmas[i].tamanho_max * sizeof(*turmas[i].alunos));
            for(int j = 0; j < atoi(size); j ++){
                strcpy(turmas[i].alunos[j],"");
            }
            strcpy(turmas[i].professor,username);
            strcpy(turmas[i].nome, name);

            //GERAR MULTICAST IP//

            unsigned long ip = htonl(0xEF000000 + i);
            struct in_addr ip_addr;
            ip_addr.s_addr = ip;

            char *ip_str = inet_ntoa(ip_addr);
            strcpy(turmas[i].multicast,ip_str);

            /////////////////////

            sprintf(message, "OK %s", turmas[i].multicast);

            write(socket_client, message, strlen(message));

            return;         
        }
    }
    strcpy(message,"REJECTED\n");
    write(socket_client, message, strlen(message) + 1);
}

void list_classes(int socket_client){
    char message[BUF_SIZE], buff[BUF_SIZE];
    memset(message, 0, sizeof(message));
    for(int i = 0; i < MAX_TURMAS; i++){
        if(turmas[i].tamanho_atual != -1) {
            if(turmas[i].tamanho_atual < turmas[i].tamanho_max){
                sprintf(buff, "%s | ", turmas[i].nome);
                strcat(message, buff);
            }
        }
    }

    if(strlen(message) == 0){
        strcpy(message, "Não existem turmas disponíves!\n");
        write(socket_client, message, strlen(message) + 1);
        return;
    }

    write(socket_client, message, strlen(message) + 1);
    memset(message, 0, sizeof(message));

    sprintf(message, "Turmas listada em cima!\n");
    write(socket_client, message, strlen(message) + 1);
}

void list_subscribed (int socket_client, char *nome){
    char message[BUF_SIZE], buff[BUF_SIZE];
    memset(message, 0, sizeof(message));
    for(int i = 0; i < MAX_TURMAS; i++){
        if(turmas[i].tamanho_max != -1){
            for(int k = 0; k < turmas[i].tamanho_max; k++){
                if(strcmp(turmas[i].alunos[k],nome) == 0){
                    sprintf(buff,"%s ",turmas[i].nome);
                    strcat(message, buff);
                    break;
                }
            }
        }
    }

    if(strlen(message) == 0){
        strcpy(message, "Não está inscrito em nenhuma turma");
        write(socket_client, message, strlen(message) + 1);
        return;
    }

    write(socket_client, message, strlen(message) + 1);
    memset(message, 0, sizeof(message));

    sprintf(message, "Turmas em que está subscrito listada em cima!\n");
    write(socket_client, message, strlen(message) + 1);
}

void subscribe_class(int socket_client, char *nome_turma, char *nome_aluno){
    char message[BUF_SIZE];
    memset(message, 0, sizeof(message));
    for(int i = 0; i < MAX_TURMAS; i++){
        if(strcmp(nome_turma,turmas[i].nome) == 0){
            for(int k = 0; k < turmas[i].tamanho_max; k++){
                if(strcmp(turmas[i].alunos[k], "") == 0){
                    strcpy(turmas[i].alunos[k], nome_aluno);
                    turmas[i].tamanho_atual++;
                    sprintf(message, "ACCEPTED %s", turmas[i].multicast);
                    write(socket_client, message, strlen(message));
                    return;
                }
            }
        }
    }
    strcpy(message,"REJECTED\n");
    write(socket_client, message, strlen(message));
}

void sendToMulticast(int client_fd, char *nome, char *message) {
    for (int i = 0; i < MAX_TURMAS; i++) {
        if (strcmp(turmas[i].nome, nome) == 0 && turmas[i].tamanho_atual > 0) {
        int socketMulticast;
        struct sockaddr_in addrmulticast;

        if ((socketMulticast = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
            perror("Error in socket");
            return;
        }

        memset(&addrmulticast, 0, sizeof(addrmulticast));
        addrmulticast.sin_family = AF_INET;
        addrmulticast.sin_addr.s_addr = inet_addr(turmas[i].multicast);
        addrmulticast.sin_port = htons(MULTICAST_PORT);

        int enable = 1;
        if(setsockopt(socketMulticast, IPPROTO_IP, IP_MULTICAST_TTL, &enable, sizeof(enable)) < 0) {
            perror("setsockopt");
        }

        if (sendto(socketMulticast, message, strlen(message), 0, (struct sockaddr *) &addrmulticast, sizeof(addrmulticast)) < 0) {
            perror("sendto");
        }

        close(socketMulticast);
        return;
        }
    }

    write(client_fd, "Turma não foi encontrada ou não tem alunos!\n", strlen("Turma não foi encontrada ou não tem alunos!\n") + 1);
    
}

void *process_client(void *arg) {
    int client_socket = *((int *) arg);

    int nread;
    char buffer[BUF_SIZE], mensagem[BUF_SIZE], comando[BUF_SIZE], username[BUF_SIZE], 
    password[BUF_SIZE], role[BUF_SIZE], args[BUF_SIZE], args1[BUF_SIZE];

    write(client_socket, "Bem-Vindo ao Servior! (LOGIN <username> <password>): ", 1 + strlen("Bem-Vindo ao Servior! (LOGIN <username> <password>): "));
    nread = read(client_socket, buffer, BUF_SIZE - 1);
    buffer[nread] = '\0';
    buffer[strcspn(buffer, "\r\n")] = 0;

    if(sscanf(buffer, "%s %s %s", comando, username, password) == 3) {
        if(strcmp(comando,"LOGIN") == 0) {
            if(login(username, password, role) == 1) {

                write(client_socket, "OK", 1 + strlen("OK"));

                role[strcspn(role, "\r\n")] = 0;
                printf("Usuário %s conectado!\n", username);

                if(strcmp(role, "aluno") == 0) {
                    write(client_socket, opc_alunos, 1 + strlen(opc_alunos));
                } else if(strcmp(role, "professor") == 0){
                    write(client_socket, opc_professores, 1 + strlen(opc_professores));
                }

                do {
                    nread = read(client_socket, buffer, BUF_SIZE-1);
                    buffer[nread] = '\0';
                    buffer[strcspn(buffer, "\r\n")] = 0;
                    if(strcmp(role,"aluno") == 0) {
                        if(sscanf(buffer,"%s %s",comando,args) != 2) {
                            if(strcmp(buffer,"LIST_CLASSES") == 0) {
                                list_classes(client_socket);
                            }else if(strcmp(buffer,"LIST_SUBSCRIBED") == 0) {
                                list_subscribed(client_socket, username);
                            } else{
                                write(client_socket, "Comando Inválido!", strlen("Comando Inválido!"));
                            }
                        }else {
                            if(strcmp(comando, "SUBSCRIBE_CLASS") == 0) {
                                subscribe_class(client_socket, args, username);
                            }else {
                                write(client_socket, "Comando Inválido!", strlen("Comando Inválido!"));
                            }                            
                        }
                        
                    } else{
                        if(sscanf(buffer, "%s %s %s", comando, args, args1) != 3){
                            if(strcmp(buffer,"LIST_CLASSES") == 0 ) {
                                list_classes(client_socket);
                            }else if(strcmp(buffer,"LIST_SUBSCRIBED") == 0) {
                                list_subscribed(client_socket, username);
                            } else{
                                write(client_socket, "Comando Inválido!", strlen("Comando Inválido!"));
                            }
                        }else {
                            if(strcmp(comando,"CREATE_CLASS") == 0) {
                                create_class(client_socket, args, args1, username);
                            }else if(strcmp(comando,"SEND") == 0) {
                                sendToMulticast(client_socket, args, args1);
                            } else{
                                write(client_socket, "Comando Inválido!", strlen("Comando Inválido!"));
                            }
                        }
                    }
                    fflush(stdout);
                }while(nread > 0);
            }else {
                sprintf(mensagem, "REJECTED");
                write(client_socket , mensagem, 1 + strlen(mensagem));
            }
        }else {
            write(client_socket, "Comando Inválido!", strlen("Comando Inválido!"));
        }
    }else{
        write(client_socket, "Comando Inválido ou Numero de Argumentos Inválido!", strlen("Comando Inválido ou Numero de Argumentos Inválido!"));
    }

    userCounter--;
    close(client_socket);
    pthread_exit(NULL);
}

void *tcp_connection(void *arg){ //void *arg permite que sejam passados qualquer tipo de variavel
    int tcp_fd = *((int *) arg); //Vai buscar o valor apontado pelo ponteiro arg (Em int)

    while(1) {
        struct sockaddr_in client_addr;
        socklen_t client_addr_size = sizeof(client_addr);

        int *client = malloc(sizeof(int));

        if ((*client = accept(tcp_fd, (struct sockaddr *)&client_addr, &client_addr_size)) < 0) {
            perror("Error in accept");
            free(client); // Libertar a memória se o accept falhar
            continue;
        }

        clients[userCounter].client = client;
        userCounter++;

        pthread_t client_thread;

        clients[userCounter].client_thread = client_thread;

        if(pthread_create(&client_thread, NULL, process_client, client) != 0) {
            perror("Failed to create thread for client");
            close(*client);
            free(client); // Libertar a memória se a criação da thread falhar
        }
        
        pthread_detach(client_thread);
    }
}

void *udp_connection(void *arg){
    int udp_fd = *((int *)arg);

    struct sockaddr_in si_minha, si_outra;

    int recv_len;
	socklen_t slen = sizeof(si_outra);
    char buffer[BUF_SIZE], comando[BUF_SIZE], username[BUF_SIZE], password[BUF_SIZE], role[BUF_SIZE], args[BUF_SIZE];

	// Cria um socket para recepção de pacotes UDP
	if((udp_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1){
		perror("Erro na criação do socket");
	}

    // Preenchimento da socket address structure
	si_minha.sin_family = AF_INET;
	si_minha.sin_port = htons(PORTO_CONFIG);
	si_minha.sin_addr.s_addr = htonl(INADDR_ANY);

	// Associa o socket à informação de endereço
	if(bind(udp_fd,(struct sockaddr*)&si_minha, sizeof(si_minha)) == -1){
		perror("Erro no bind");
	}

    if((recv_len = recvfrom(udp_fd, buffer, BUF_SIZE, 0, (struct sockaddr *) &si_outra, (socklen_t *)&slen)) == -1) {
        erro("Erro no recvfrom");
    }

    sendMessage(udp_fd, "Bem-Vindo ao Servior! (LOGIN <username> <password>): \n", si_outra, slen);

    while(1){
        
        if((recv_len = recvfrom(udp_fd, buffer, BUF_SIZE, 0, (struct sockaddr *) &si_outra, (socklen_t *)&slen)) == -1) {
            erro("Erro no recvfrom");
        }

        buffer[recv_len]='\0';
        buffer[strcspn(buffer, "\r\n")] = 0;

        if(sscanf(buffer,"%s %s %s", comando, username, password) == 3) {
            if(strcmp(comando,"LOGIN") == 0) {
                if(login(username, password, role) == 1){

                    role[strcspn(role, "\r\n")] = 0;

                    if(strcmp(role, "administrador") == 0) {
                        sendMessage(udp_fd, "OK\n", si_outra, slen);
                        sendMessage(udp_fd, opc_administrador, si_outra, slen);
                        
                        while(1) {
                            
                            if((recv_len = recvfrom(udp_fd, buffer, BUF_SIZE, 0, (struct sockaddr *) &si_outra, (socklen_t *)&slen)) == -1) {
                                erro("Erro no recvfrom");
                            }
	
                            buffer[recv_len]='\0';
                            buffer[strcspn(buffer, "\r\n")] = 0;

                            if(sscanf(buffer, "%s %s %s %s", comando, username, password, role) == 4) {
                                if(strcmp(role, "administrador") == 0 || strcmp(role, "professor") == 0 || strcmp(role, "aluno") == 0) {
                                    if(strcmp(comando, "ADD_USER") == 0) {
                                        add_user(username, password, role, udp_fd, si_outra, slen);
                                    }else {
                                        sendMessage(udp_fd, "Comando Inválido!\n", si_outra, slen);
                                    }
                                }else {
                                    sendMessage(udp_fd, "Cargo Fornecido Inválido!\n", si_outra, slen);
                                }
                            }else if(sscanf(buffer, "%s %s", comando, args) == 2) {
                                if(strcmp(comando, "DEL") == 0) {
                                    del_user(args, udp_fd, si_outra, slen);
                                }else {
                                    sendMessage(udp_fd, "Comando Inválido!\n", si_outra, slen);
                                }
                            }else {
                                if(strcmp(comando, "LIST") == 0) {
                                    list_users(udp_fd, si_outra, slen);
                                }else if(strcmp(comando, "QUIT_SERVER") == 0) {
                                    cleanup();
                                    exit(1);
                                }else {
                                    sendMessage(udp_fd, "Comando Inválido ou Numero de Argumentos Inválido!!\n", si_outra, slen);
                                }
                            }
                        }
                    }else {
                        sendMessage(udp_fd, "O usuário não possui permissões de administrador, tente outra conta!\n", si_outra, slen);
                    }
                }else {
                    sendMessage(udp_fd, "REJECTED\n", si_outra, slen);
                    break;
                }
            } else{
                sendMessage(udp_fd, "Comando Inválido!\n", si_outra, slen);
            }
        }else if(strcmp(buffer, "X") == 0) {
            continue;
        } else{
            sendMessage(udp_fd, "Comando Inválido ou Numero de Argumentos Inválido!\n", si_outra, slen);
        }
    }

    close(udp_fd);
    return NULL;
}

int main(int argc, char *argv []) {
    if(argc != 4) {
        printf("Invalid input <./filename> <PORTO_TURMAS> <PORTO_CONFIG> <ficheiro config>\n");
        exit(1);
    }

    signal(SIGINT, sign_handler);

    users = malloc(sizeof(User) * NUSERS);

    for(int i = 0; i < NUSERS; i ++){
        strcpy(users[i].username,"");
        strcpy(users[i].password,"");
        strcpy(users[i].role,"");
    }

    clients = malloc(sizeof(client_threads) * NUSERS);

    struct sockaddr_in tcp_addr;

    PORTO_TURMAS = atoi(argv[1]);
    PORTO_CONFIG = atoi(argv[2]);
    strcpy(ficheiro_config, argv[3]);

    loadUsers();

    for(int i = 0; i < MAX_TURMAS; i++){
        turmas[i].tamanho_max = -1;
    }

    //////////////////////////////////TCP//////////////////////////////////////////

    bzero((void *) &tcp_addr, sizeof(tcp_addr));
    tcp_addr.sin_family = AF_INET;
    tcp_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    tcp_addr.sin_port = htons(PORTO_TURMAS);
 
    if ( (tcp_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	erro("Ao criar a socket TCP");

    if ( bind(tcp_fd,(struct sockaddr*)&tcp_addr,sizeof(tcp_addr)) < 0)
	erro("na funcao bind");

    if (pthread_create(&thread_tcp, NULL, tcp_connection, (void *)&tcp_fd) != 0) {
        perror("Error creating TCP thread");
        exit(EXIT_FAILURE);
    }

    if(listen(tcp_fd, 5) < 0) erro("na funcao listen");

    if(pthread_create(&thread_udp, NULL, udp_connection, (void *)&udp_fd) != 0){
        perror("Error creating UDP thread");
        exit(EXIT_FAILURE);
    }

    pthread_join(thread_tcp, NULL);
    pthread_join(thread_udp, NULL);

    return 0;
}