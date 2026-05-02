#include "mensajes.h"
#include "almacenamiento.h"
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

// Estructura para pasar múltiples argumentos al hilo
struct ThreadArgs {
    int socket;
    char ip[16];
};



void conexion(void *arg) {
    int my_sc;
    char ip_cliente[16];
    struct ThreadArgs *mis_args = (struct ThreadArgs *)arg;

    pthread_mutex_lock(&m);

    // Copiamos ambos valores del struct
    my_sc = mis_args->socket;
    strcpy(ip_cliente, mis_args->ip);

    copiado = 1;
    pthread_cond_signal(&cv);
    pthread_mutex_unlock(&m);

    /*Variables para guardar los datos recibidos*/
    char operacion[MAX_NAME], nombre[MAX_NAME], puerto_str[20], destinatario[MAX_NAME], mensaje[MAX_MSG];
    int op, puerto_int;
    ssize_t err;
    unsigned char resultado;

    while (1) {
        /*Primero, obtenemos la operación que se debe realizar*/
        err = readLine(my_sc, operacion, sizeof(operacion));
        if (err <= 0) {
            // Si es 0, es un cierre limpio del cliente. Si es < 0, es un error real.
            if (err < 0) {
                printf("Error en recepcion\n");
            }
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
                /*Leemos el nombre del usuario*/
                err = readLine(my_sc,nombre,sizeof(nombre));
                if (err <= 0) {
                    printf("Error en recepcion\n");
                    close(my_sc);
                    break;
                }

                /*Leemos el puerto del usuario*/
                err = readLine(my_sc,puerto_str,sizeof(puerto_str));
                if (err <= 0) {
                    printf("Error en recepcion\n");
                    close(my_sc);
                    break;
                }
                puerto_int = atoi(puerto_str);

                resultado = conectar_usuario(nombre,puerto_int,ip_cliente);
                sendMessage(my_sc, (char *)&resultado,1);
                break;

            case OP_DISCONNECT:
                /*Leemos el nombre del usuario*/
                err = readLine(my_sc,nombre,sizeof(nombre));
                if (err <= 0) {
                    printf("Error en recepcion\n");
                    close(my_sc);
                    break;
                }

                resultado = desconectar_usuario(nombre,ip_cliente);
                sendMessage(my_sc, (char *)&resultado,1);
                break;
                
            case OP_SEND:
                /*Leemos el nombre del usuario*/
                err = readLine(my_sc,nombre,sizeof(nombre));
                if (err <= 0) {
                    printf("Error en recepcion\n");
                    close(my_sc);
                    break;
                }
                /*Leemos el nombre del destinatario*/
                err = readLine(my_sc,destinatario,sizeof(destinatario));
                if (err <= 0) {
                    printf("Error en recepcion\n");
                    close(my_sc);
                    break;
                }
                /*Leemos el mensaje*/
                err = readLine(my_sc,mensaje,sizeof(mensaje));
                if (err <= 0) {
                    printf("Error en recepcion\n");
                    close(my_sc);
                    break;
                }

                /* Preparamos los punteros necesarios*/
                unsigned int id_asignado = 0; 

                resultado = enviar_mensaje(nombre, destinatario, mensaje, &id_asignado);

                // Enviamos los datos al cliente
                sendMessage(my_sc, (char *)&resultado,1);
                if(resultado == 0){
                    char id_str[20]; 
                    // Convertimos id_asignado a texto y lo guardamos en id_str
                    sprintf(id_str, "%u", id_asignado);
                    sendMessage(my_sc, id_str,strlen(id_str) + 1);
                }
                
                break;

            case OP_USERS:
                /*Leemos el nombre del usuario*/
                err = readLine(my_sc,nombre,sizeof(nombre));
                if (err <= 0) {
                    printf("Error en recepcion\n");
                    close(my_sc);
                    break;
                }

                /* Preparamos los punteros necesarios*/
                int n_conectados = 0;           // El numero de usuarios conectados
                char *p_conectados = NULL;         /* Un puntero a una direccion de memoria donde se guardan los nombres de los 
                                                usuarios conectados separados por ';' */ 
                resultado = users(nombre,&n_conectados,&p_conectados);

                // Enviamos los datos al cliente
                sendMessage(my_sc, (char *)&resultado,1);
                if(resultado == 0){
                    char num_str[20]; 
                    // Convertimos num_conectados a texto y lo guardamos en num_str
                    sprintf(num_str, "%d", n_conectados);
                    sendMessage(my_sc, num_str,strlen(num_str) + 1);

                    if (n_conectados > 0 && p_conectados != NULL) {
                        // strtok saca el primer nombre (hasta el primer ';')
                        char *trozo = strtok(p_conectados, ";");
                        
                        while (trozo != NULL) {
                            // Enviamos este nombre solo
                            sendMessage(my_sc, trozo, strlen(trozo) + 1);
                            
                            // Le pedimos a strtok que nos dé el siguiente nombre
                            trozo = strtok(NULL, ";");
                        }
                        
                        // Como todo estaba en un solo bloque de memoria, hacemos un solo free
                        free(p_conectados);
                    }
                }
                
                break;

            case OP_SENDATTACH:
              
            default:
                perror("El código de operación no es válido");
                resultado = 2;
                sendMessage(my_sc, (char *)&resultado, 1);
        }
    }
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

    struct ThreadArgs args_hilo; // Creamos la estructura para pasar los argumentos a los hilos

    while (1) {
        // Esperamos a recibir alguna peticion de un cliente
        sc = accept(sd, (struct sockaddr *)&client_addr, (socklen_t *)&size);

        if (sc == -1) {
            printf("Error en accept\n");
            return -1;
        }

        // Preparamos los argumentos para el hilo
        args_hilo.socket = sc;
        strcpy(args_hilo.ip, inet_ntoa(client_addr.sin_addr));

        if (pthread_create(&thid, &attr, (void *)&conexion, (void *)&args_hilo) < 0) {
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