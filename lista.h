#ifndef LISTA_H
#define LISTA_H

#include <stdlib.h>     
#include <stdio.h>      
#include <string.h>     
#include <ctype.h>      
#include <time.h>       

typedef struct{ // defenição da estrutura que vai guardar a informação geral da reserva
    char *nome,*password, *role;
}User;

typedef struct noLista{ // definição do nó da lista, contem informação sobre a reserva e um ponteiro para o proximo nó da lista
    Reserva itemLista;
    struct noLista *prox;
} noLista;

typedef noLista *userList; // definição da lista como um ponteiro que aponta para o primeiro elemento (cabeçalho da lista)

userList cria ();                 
int vazia (userList lista);       
void destroi (userList lista);  
void elimina (userList lista, llu chave);     
void insere (userList lista, User user);     
userList pesquisa (userList lista, llu chave);  
void imprime (userList lista,int reserva);    
void inverteLista(userList lista);            

#endif