#include "lista.h"

userList cria(){
    userList lista;
    User user = {" "," ",{0,0,0},{{0,0},{0,0}},0,0,0};
    lista = (userList) malloc(sizeof(noLista));
    if(lista != NULL){
        lista->itemLista = user;
        lista->prox = NULL;
    }
    return lista;
}

int vazia(userList lista){
    return lista->prox == NULL;
}

void destroi(userList lista){
    userList temp;
    while(!vazia(lista)){
        temp=lista;
        lista=lista->prox;
        free(temp);
    }
    free(lista);
}

void elimina(userList lista, llu chave){
    userList ant,actual;
    procura_primeiro(lista,chave,&ant,&actual);
    if(actual != NULL && actual->itemLista.referencia_inicial == chave){
        ant->prox=actual->prox;
        free(actual);
    }else{
        printf("Elemento não encontrado.\n");
    }
}

void insere (userList lista, User user){
    userList no, ant, inutil;
    no = (userList) malloc (sizeof (noLista));
    if (no != NULL){
        no->itemLista = user;
        procura_ultimo(lista, user.referencia_inicial, &ant, &inutil);
        no->prox = ant->prox;
        ant->prox = no;
    }
}

userList pesquisa(userList lista, llu chave){
    userList ant,actual;
    procura_primeiro(lista,chave,&ant,&actual);
    return actual;
}

void imprime(userList lista, int reserva) {
    userList aux = lista->prox;
    system("clear");
    while (aux != NULL) {
        if(reserva){
            printf("- Reserva -\n");
        }else{
            printf("- Pre-Reserva -\n");
        }
        printf("> Serviço: ");
        if(aux->itemLista.serviço==1){
            printf("Lavagem <\n");
        }else{
            printf("Manutenção <\n");
        }
        printf("Nome: %s\nContacto: %s\nData: %02d/%02d/%d\nHorario: %02d:%02d as %02d:%02d\n\n", 
            aux->itemLista.nome,aux->itemLista.numero, aux->itemLista.data.dia, aux->itemLista.data.mes, aux->itemLista.data.ano,
            aux->itemLista.hora.inicio.hora, aux->itemLista.hora.inicio.minuto, aux->itemLista.hora.fim.hora, aux->itemLista.hora.fim.minuto);
        aux = aux->prox;
    }
    printf("\n");
}