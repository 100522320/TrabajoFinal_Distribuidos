#include "claves.h"
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

void conexion(void *sc) {
  int my_sc;

  pthread_mutex_lock(&m);
  my_sc = *(int *)sc;
  copiado = 1;
  pthread_cond_signal(&cv);
  pthread_mutex_unlock(&m);

  char str_op[MAX_MESS], key[MAX_MESS], value1[MAX_MESS], res[MAX_MESS]; // Variables para recibir y enviar los datos
  int err, op, N_value2, resultado;
  float V_value2[MAX_VEC];
  struct Paquete value3;

  while (1) {
    /*Primero, obtenemos el número de operación*/
    err = leerDato(my_sc, "int", str_op);
    if (err == -1) {
      printf("Error en recepcion\n");
      close(my_sc);
      break;
    }

    op = atoi(str_op);

    /* Ejecutamos la petición del cliente y preparar respuesta */
    switch(op){
      case DESTROY:
        resultado = destroy();
        sprintf(res, "%d;", resultado);
        write(my_sc, res, strlen(res));
        break;
      case SET_VALUE:
      {
        /*Recogemos la información necesaria para la operación*/

        /*Obtenemos la clave de la tupla*/
        err = leerDato(my_sc, "str", key);
        if (err == -1) {
          printf("Error en recepcion\n");
          close(my_sc);
          break;
        }

        err = recopilarParametros(my_sc, value1, &N_value2, V_value2, &value3);
        if (err == -1) {
          close(my_sc);
          break;
        }

        resultado = set_value(key, value1, N_value2, V_value2, value3);
        sprintf(res, "%d;", resultado);
        write(my_sc, res, strlen(res));
        break;
      }
      case GET_VALUE:
        {
        /*Creamos estructuras de datos auxiliares para no modificar el mensaje recibido*/
        char v1[MAX_STR];
        int N_v2;
        float V_v2[MAX_VEC];
        struct Paquete v3;

        /*Recogemos la información necesaria para la operación*/
        err = leerDato(my_sc, "str", key);
        if (err == -1) {
          close(my_sc);
          break;
        }

        resultado = get_value(key, v1, &N_v2, V_v2, &v3);
        sprintf(res, "%d;", resultado);
        write(my_sc, res, strlen(res));

        /*Actualizamos la respuesta si ha ido todo bien*/
        if (resultado == 0) {
          if (enviarParametros(my_sc, v1, &N_v2, V_v2, &v3) < 0){
            close(my_sc);
            break;
          }
        }
        break;
      }
      case MODIFY_VALUE:
        /*Recogemos la información necesaria para la operación*/

        /*Obtenemos la clave de la tupla*/
        err = leerDato(my_sc, "str", key);
        if (err == -1) {
          printf("Error en recepcion\n");
          close(my_sc);
          break;
        }

        err = recopilarParametros(my_sc, value1, &N_value2, V_value2, &value3);
        if (err == -1) {
          close(my_sc);
          break;
        }

        resultado = modify_value(key, value1, N_value2, V_value2, value3);
        sprintf(res, "%d;", resultado);
        write(my_sc, res, strlen(res));
        break;
      case DELETE_KEY:
        /*Recogemos la información necesaria para la operación*/

        /*Obtenemos la clave de la tupla*/
        err = leerDato(my_sc, "str", key);
        if (err == -1) {
          printf("Error en recepcion\n");
          close(my_sc);
          break;
        }

        resultado = delete_key(key);
        sprintf(res, "%d;", resultado);
        write(my_sc, res, strlen(res));
        break;

      case EXIST:
        /*Recogemos la información necesaria para la operación*/

        /*Obtenemos la clave de la tupla*/
        err = leerDato(my_sc, "str", key);
        if (err == -1) {
          printf("Error en recepcion\n");
          close(my_sc);
          break;
        }

        resultado = exist(key);
        sprintf(res, "%d;", resultado);
        write(my_sc, res, strlen(res));
        break;

      default:
        perror("El código de operación no es válido");
        resultado = -1;
        sprintf(res, "%d;", resultado);
        write(my_sc, res, strlen(res));
	  }
  }
  printf("conexion cerrada\n");
}



int main(int argc, char *argv[]) {
  if (argc != 2) {
    printf("Usage: server <serverPort>\n");
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
  server_addr.sin_port = htons(atoi(argv[1]));

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

  while (1) {
    printf("esperando conexion\n");
    sc = accept(sd, (struct sockaddr *)&client_addr, (socklen_t *)&size);

    if (sc == -1) {
      printf("Error en accept\n");
      return -1;
    }
    printf("conexion aceptada de IP: %s   Puerto: %d\n",
           inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

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
