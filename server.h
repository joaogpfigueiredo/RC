#ifndef _SERVER_H
#define _SERVER_H

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
#include <signal.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/stat.h>

#define BUF_SIZE 1024
#define NUSERS 100
#define USER_LENGTH 16
#define ROLE_LENGTH 16
#define PASSWORD_LENGTH 20
#define MAX_LINE_LENGTH 300
#define MAX_TURMAS 20
#define MULTICAST_PORT 8080
#define opc_alunos "OK\nLista de comandos para Alunos:\n> LIST_CLASSES\n> LIST_SUBSCRIBED\n> SUBSCRIBE_CLASS <nome>\n> CTRL+C para encerrar"
#define opc_professores "OK\nLista de comandos para Professores:\n> LIST_CLASSES\n> LIST_SUBSCRIBED\n> CREATE_CLASS <nome> <size>\n> SEND <nome> <mensagem>\n> CTRL+C para encerrar"
#define opc_administrador "OK\nLista de comandos para Administradores:\n> LIST\n> QUIT_SERVER\n> DEL <username>\n> ADD_USER <username> <password> <aluno|professor|administrador>\n> CTRL+C para encerrar\n"

typedef struct {
    char username[USER_LENGTH];
    char password[PASSWORD_LENGTH];
    char role[ROLE_LENGTH];
} User;

typedef struct {
    int tamanho_max;
    int tamanho_atual;
    char nome[MAX_LINE_LENGTH];
    char professor[USER_LENGTH];
    char (*alunos)[USER_LENGTH]; 
    char multicast[128]; 
} Turma;

typedef struct {
    pthread_t client_thread;
    int *client;
} client_threads;

User *users;
client_threads *clients;

Turma turmas[MAX_TURMAS];

pthread_t thread_tcp, thread_udp;

int running = 1, userCounter = 0;
int PORTO_TURMAS, PORTO_CONFIG;

int tcp_fd;

int udp_fd, recv_len;
struct sockaddr_in si_minha, si_outra;
socklen_t slen;

char ficheiro_config[MAX_LINE_LENGTH];

#endif