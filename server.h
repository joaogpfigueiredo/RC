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

#define BUF_SIZE 1024
#define USER_LENGTH 16
#define ROLE_LENGTH 16
#define PASSWORD_LENGTH 20
#define MAX_LINE_LENGTH 300
#define opc_alunos "Lista de comandos para Alunos:\n> LIST_CLASSES\n> LIST_SUBSCRIBED\n> SUBSCRIBE_CLASS <nome>\n> CTRL+C para encerrar"
#define opc_professores "Lista de comandos para Professores:\n> LIST_CLASSES\n> LIST_SUBSCRIBED\n> CREATE_CLASS <nome> <size>\n> SEND <nome> <mensagem>\n> CTRL+C para encerrar"
#define opc_administrador "Lista de comandos para Administradores:\n> LIST\n> QUIT_SERVER\n> DEL <username>\n> ADD_USER <username> <password> <aluno|professor|administrador>\n> CTRL+C para encerrar\n"

typedef struct {
    char username[USER_LENGTH];
    char password[PASSWORD_LENGTH];
    char role[ROLE_LENGTH];
} User;

User *users;

pthread_t thread_tcp, thread_udp;

int PORTO_TURMAS, PORTO_CONFIG;
int tcp_fd, udp_fd;
char ficheiro_config[MAX_LINE_LENGTH];

#endif