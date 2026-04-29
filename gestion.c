#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "gestion.h"
#include "almacenamiento.h"
#include <stdlib.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

nodo_clientes *head = NULL;
// Definición del mutex para proteger la lista enlazada
pthread_mutex_t mutex_lista = PTHREAD_MUTEX_INITIALIZER;

int op_a_int(char *operacion){
    /*Cogemos la operacion recibida y la comparamos con las distintas opciones 
        para devolver el numero correspondiente*/

    if (strcmp(operacion, "REGISTER") == 0) {
        return OP_REGISTER;
    } else if (strcmp(operacion, "UNREGISTER") == 0) {
        return OP_UNREGISTER;
    } else if (strcmp(operacion, "CONNECT") == 0) {
        return OP_CONNECT;
    } else if (strcmp(operacion, "DISCONNECT") == 0) {
        return OP_DISCONNECT;
    } else if (strcmp(operacion, "SEND") == 0) {
        return OP_SEND;
    } else if (strcmp(operacion, "USERS") == 0) {
        return OP_USERS;
    } else if (strcmp(operacion, "SENDATTACH") == 0) {
        return OP_SENDATTACH;
    }
    // Si la cadena no coincide con ninguna operación válida
    return -1;
}

int existe_usuario(char *nombre){
    /*Devuelve 1 si existe un usuario registrado con ese nombre y 0 en caso contrario.
        Se asume que antes de llamarla ya se habrá hecho lock del mutex*/
    nodo_clientes *nodo = head;

    while (nodo){
        if (strcmp(nodo->nombre, nombre) == 0){
            return 1;
        }
        
        nodo = nodo->next;
    }
    return 0;
}

unsigned char registrar_usuario(char *nombre){
    pthread_mutex_lock(mutex_lista);

    /*Comprobamos que no haya otro usuario resgistrado con el mismo nombre*/
    if (existe_usuario(nombre) == 1){
        // Imprimimos el mensaje de error
        printf("s> REGISTER %s FAIL\n", nombre);
        pthread_mutex_unlock(mutex_lista);
        return 1;
    }
    
    /* Añadimos el nuevo usuario*/
    nodo_clientes *nuevo_cliente = (nodo_clientes *)malloc(sizeof(nodo_clientes));
    if (nuevo_cliente == NULL) {
        perror("Error asignando memoria para el nuevo cliente");
        pthread_mutex_unlock(&mutex_lista);
        return 2; 
    }

    strcpy(nuevo_cliente->nombre,nombre);
    nuevo_cliente->conectado = 0;
    memset(nuevo_cliente->ip, 0, MAX_IP);
    nuevo_cliente->puerto = 0;
    nuevo_cliente->ultimo_id = 0;
    nuevo_cliente->mensajes_pendientes = NULL;
    nuevo_cliente->next = head;

    /*Actualizamos head*/
    head = nuevo_cliente;
    
    // Imprimimos el mensaje de éxito
    printf("s> REGISTER %s OK\n", nombre);

    pthread_mutex_unlock(mutex_lista);
    return 0;
}