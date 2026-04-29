#include "mensajes.h"
#include "gestion.h"
#include <pthread.h>
#include <stdbool.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>


pthread_mutex_t m;
pthread_cond_t cv;
int copiado = 0;



void conexion(void *sc) {
    int my_sc;

    pthread_mutex_lock(&m);
    my_sc = *(int *)sc;
    copiado = 1;
    pthread_cond_signal(&cv);
    pthread_mutex_unlock(&m);

    /*Variables para guardar los datos recibidos*/
    char operacion[MAX_NAME], nombre[MAX_NAME];
    int op;
    ssize_t err;
    unsigned char resultado;

    while (1) {
        /*Primero, obtenemos la operación que se debe realizar*/
        err = readLine(my_sc,operacion,sizeof(operacion));
        if (err <= 0) {
            printf("Error en recepcion\n");
            close(my_sc);
            break;
        }

        /* Transformamos el codigo de operacion de str a int*/
        op = op_a_int(operacion);
        if (op == -1){
            printf("Error: no se reconoce esa operacion\n");
            close(my_sc);
            break;
        }

        /* Ejecutamos la petición del cliente */
        switch(op){
            case OP_REGISTER:
                /*Leemos el nombre del usuario*/
                err = readLine(my_sc,nombre,sizeof(nombre));
                if (err <= 0) {
                    printf("Error en recepcion\n");
                    close(my_sc);
                    break;
                }

                resultado = registrar_usuario(nombre);
                sendMessage(my_sc, (char *)&resultado,1);
                break;
                
            case OP_UNREGISTER:
                /*Leemos el nombre del usuario*/
                err = readLine(my_sc,nombre,sizeof(nombre));
                if (err <= 0) {
                    printf("Error en recepcion\n");
                    close(my_sc);
                    break;
                }

                resultado = dar_de_baja_usuario(nombre);
                sendMessage(my_sc, (char *)&resultado,1);
                break;

            case OP_CONNECT:
                
            case OP_DISCONNECT:
                
            case OP_SEND:

            case OP_USERS:

            case OP_SENDATTACH:
              
            default:
                perror("El código de operación no es válido");
                resultado = 2;
                sendMessage(my_sc, (char *)&resultado, 1);
        }
    }
    printf("s> conexion cerrada\n");
}




int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: ./server -p <port>\n");
        exit(0);
    }

    struct sockaddr_in server_addr, client_addr;
    socklen_t size;
    int sd, sc;
    int err;
    int val;
    pthread_t thid;

    sd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sd < 0) {
        perror("Error in socket");
        exit(1);
    }

    val = 1;
    err = setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, (char *)&val, sizeof(int));
    if (err < 0) {
        perror("Error in opction");
        exit(1);
    }

    bzero((char *)&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(atoi(argv[2]));

    err = bind(sd, (const struct sockaddr *)&server_addr, sizeof(server_addr));
    if (err == -1) {
        printf("Error en bind\n");
        return -1;
    }

    err = listen(sd, SOMAXCONN);
    if (err == -1) {
        printf("Error en listen\n");
        return -1;
    }

    size = sizeof(client_addr);

    pthread_mutex_init(&m, NULL);
    pthread_cond_init(&cv, NULL);

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    // Imprimimos que el server ya funciona
    printf("s> init server %s:%d\n", inet_ntoa(server_addr.sin_addr), ntohs(server_addr.sin_port));

    while (1) {
        printf("s> \n");

        // Esperamos a recibir alguna peticion de un cliente
        sc = accept(sd, (struct sockaddr *)&client_addr, (socklen_t *)&size);

        if (sc == -1) {
            printf("Error en accept\n");
            return -1;
        }

        if (pthread_create(&thid, &attr, (void *)&conexion, (void *)&sc) < 0) {
        printf("Error al lanzar el hilo\n");
        break;
        }

        pthread_mutex_lock(&m);
        while (copiado == 0) {
            pthread_cond_wait(&cv, &m);
        }
        copiado = 0;
        pthread_mutex_unlock(&m);
    }

    pthread_mutex_destroy(&m);
    pthread_cond_destroy(&cv);

    close(sd);
    return (0);
}