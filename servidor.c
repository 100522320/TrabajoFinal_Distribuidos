#include "mensajes.h"
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
    printf("s> init server %s:%d\n", inet_ntoa(server_addr.sin_addr), ntohs(server_addr.sin_port))

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