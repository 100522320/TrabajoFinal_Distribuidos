#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "gestion.h"
#include "mensajes.h"
#include "almacenamiento.h"
#include <stdlib.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

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

nodo_clientes *existe_usuario(char *nombre){
    /*  Devuelve el puntero al nodo si existe un usuario registrado.
        Se asume que antes de llamarla ya se habrá hecho lock del mutex*/
    nodo_clientes *nodo = head;

    while (nodo){
        if (strcmp(nodo->nombre, nombre) == 0){
            return nodo;
        }
        
        nodo = nodo->next;
    }
    return NULL;
}

unsigned char registrar_usuario(char *nombre){
    pthread_mutex_lock(mutex_lista);

    /*Comprobamos que no haya otro usuario registrado con el mismo nombre*/
    nodo_clientes *cliente = existe_usuario(nombre);
    if (cliente != NULL){
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
    nuevo_cliente->before = NULL;

    /*Actualizamos head*/
    head = nuevo_cliente;
    
    // Imprimimos el mensaje de éxito
    printf("s> REGISTER %s OK\n", nombre);

    pthread_mutex_unlock(mutex_lista);
    return 0;
}

unsigned char dar_de_baja_usuario(char *nombre){
    pthread_mutex_lock(mutex_lista);

    /*Comprobamos que el usuario este registrado*/
    nodo_clientes *cliente = existe_usuario(nombre);
    if (cliente == NULL){
        // Imprimimos el mensaje de error
        printf("s> UNREGISTER %s FAIL\n", nombre);
        pthread_mutex_unlock(mutex_lista);
        return 1;
    }
    
    /* Borramos el usuario*/
    if(cliente->before != NULL){
        cliente->before->next = cliente->next;
    }
    else{
        head = cliente->next;
    }

    if(cliente->next != NULL){
        cliente->next->before = cliente->before;
    }
    
    /*Necesitamos borra tambien los mensajes pendientes de recibirs*/
    nodo_mensaje *msg_actual = cliente->mensajes_pendientes;
    while (msg_actual != NULL) {
        nodo_mensaje *aux = msg_actual;
        msg_actual = msg_actual->next;
        free(aux); // Liberamos cada mensaje
    }
    free(cliente);

    // Imprimimos el mensaje de éxito
    printf("s> UNREGISTER %s OK\n", nombre);

    pthread_mutex_unlock(mutex_lista);
    return 0;
}

unsigned char conectar_usuario(char *nombre, int puerto_cliente, char *ip_cliente){

    pthread_mutex_lock(mutex_lista);

    /*Comprobamos que el usuario este registrado y no conectado*/
    nodo_clientes *cliente = existe_usuario(nombre);
    if (cliente == NULL){
        // Imprimimos el mensaje de error
        printf("s> CONNECT %s FAIL\n", nombre);
        pthread_mutex_unlock(mutex_lista);
        return 1;
    }
    if (cliente->conectado == 1){
        // Imprimimos el mensaje de error
        printf("s> CONNECT %s FAIL\n", nombre);
        pthread_mutex_unlock(mutex_lista);
        return 2;
    }

    // Rellenamos sus datos
    cliente->puerto = puerto_cliente;
    strcpy(cliente->ip,ip_cliente);
    cliente->conectado = 1;

    // Imprimimos el mensaje de éxito
    printf("s> CONNECT %s OK\n", nombre);

    // Le mandamos todos los mensajes guardados (si hay)
    nodo_mensaje *mensaje = cliente->mensajes_pendientes;

    while(mensaje){
        // Creamos un socket TCP para hablar con el hilo de escucha del cliente
        int sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) {
            perror("Error creando socket para enviar mensaje pendiente");
            break;
        }

        // Configuramos la dirección del cliente (IP y Puerto) y nos conectamos
        struct sockaddr_in client_addr;
        client_addr.sin_family = AF_INET;
        client_addr.sin_port = htons(cliente->puerto);
        client_addr.sin_addr.s_addr = inet_addr(cliente->ip);

        if (connect(sock, (struct sockaddr *)&client_addr, sizeof(client_addr)) < 0) {
            perror("Error conectando al hilo de escucha del cliente");
            close(sock);
            break;
        }

        // Enviamos la operación
        char op[] = "SEND_MESSAGE\0"; 
        sendMessage(sock, op, strlen(op) + 1); 

        // Enviamos el remitente
        char remitente[MAX_NAME + 1];
        sprintf(remitente, "%s", mensaje->remitente);
        sendMessage(sock, remitente, strlen(remitente) + 1);

        // Enviamos el ID del mensaje como cadena de texto
        char id_str[20];
        sprintf(id_str, "%u", mensaje->id);
        sendMessage(sock, id_str, strlen(id_str) + 1);

        // Enviamos el contenido del mensaje
        char contenido[MAX_MSG + 1];
        sprintf(contenido, "%s", mensaje->contenido);
        sendMessage(sock, contenido, strlen(contenido) + 1);

        close(sock);

        printf("s> SEND MESSAGE %u FROM %s TO %s\n", mensaje->id, mensaje->remitente, cliente->nombre);

        // Buscamos al usuario que redactó el mensaje originalmente
        nodo_clientes *nodo_remitente = existe_usuario(mensaje->remitente);
        
        // Solo intentamos enviar el ACK si el remitente existe y está conectado
        if (nodo_remitente != NULL && nodo_remitente->conectado == 1) {
            int sock_ack = socket(AF_INET, SOCK_STREAM, 0);
            if (sock_ack >= 0) {
                struct sockaddr_in ack_addr;
                ack_addr.sin_family = AF_INET;
                ack_addr.sin_port = htons(nodo_remitente->puerto);
                ack_addr.sin_addr.s_addr = inet_addr(nodo_remitente->ip);

                if (connect(sock_ack, (struct sockaddr *)&ack_addr, sizeof(ack_addr)) == 0) {
                    // Enviamos la cadena "SEND MESS_ACK" indicando la operación
                    char op_ack[] = "SEND MESS_ACK\0";
                    sendMessage(sock_ack, op_ack, strlen(op_ack) + 1);

                    // Enviamos el identificador asociado al mensaje que se ha entregado
                    sendMessage(sock_ack, id_str, strlen(id_str) + 1);
                } 
                else {
                    // Si falla la conexión, el protocolo asume que se ha desconectado
                    nodo_remitente->conectado = 0;           // Cambiamos estado
                    nodo_remitente->puerto = 0;              // Limpiamos puerto
                    memset(nodo_remitente->ip, 0, MAX_IP);   // Limpiamos IP
                    perror("Error conectando al remitente para enviar ACK. Marcando como desconectado");
                }
                close(sock_ack);
            }
        }

        nodo_mensaje *aux = mensaje;
        mensaje = mensaje->next;
        free(aux);
    }
    
    // Actualizamos la lista de mensajes del cliente. 
    // Si todo fue bien, será NULL. Si hubo error a medias, apuntará al primer mensaje no enviado.
    cliente->mensajes_pendientes = mensaje;

    pthread_mutex_unlock(mutex_lista);
    return 0;
}

unsigned char desconectar_usuario(char *nombre){

    pthread_mutex_lock(mutex_lista);

    /*Comprobamos que el usuario este registrado y conectado*/
    nodo_clientes *cliente = existe_usuario(nombre);
    if (cliente == NULL){
        // Imprimimos el mensaje de error
        printf("s> DISCONNECT %s FAIL\n", nombre);
        pthread_mutex_unlock(mutex_lista);
        return 1;
    }
    if (cliente->conectado == 0){
        // Imprimimos el mensaje de error
        printf("s> DISCONNECT %s FAIL\n", nombre);
        pthread_mutex_unlock(mutex_lista);
        return 2;
    }

    // Rellenamos sus datos
    cliente->conectado = 0;           
    cliente->puerto = 0;              
    memset(cliente->ip, 0, MAX_IP);  

    // Imprimimos el mensaje de éxito
    printf("s> DISCONNECT %s OK\n", nombre);

    pthread_mutex_unlock(mutex_lista);
    return 0;
}

int users(char *nombre, int *n_conectados, char *p_conectados){
    pthread_mutex_lock(mutex_lista);

    /*Comprobamos que el usuario este registrado y conectado*/
    nodo_clientes *cliente = existe_usuario(nombre);
    if (cliente == NULL){
        // Imprimimos el mensaje de error
        printf("s> CONNECTEDUSERS %s FAIL\n", nombre);
        pthread_mutex_unlock(mutex_lista);
        return 2;
    }
    if (cliente->conectado == 0){
        // Imprimimos el mensaje de error
        printf("s> CONNECTEDUSERS %s FAIL\n", nombre);
        pthread_mutex_unlock(mutex_lista);
        return 1;
    }

    // Miramos todos los usuarios conectados actualmente
    nodo_clientes *cliente = head;
    int num_usu;
    int espacio_necesario = 0;
    while(cliente){
        if(cliente->conectado == 1){
            num_usu++; 
            espacio_necesario += strlen(cliente->nombre) + 1; // +1 para el separador y el /0 final
        }
        cliente = cliente->next;
    }

    // Reservamos memoria para esos clientes y guardamos sus nombres
    p_conectados = (char *)malloc(sizeof(char) * espacio_necesario);
    if (p_conectados == NULL){
        // Imprimimos el mensaje de error
        printf("s> CONNECTEDUSERS %s FAIL\n", nombre);
        pthread_mutex_unlock(mutex_lista);
        return 2;
    }

    cliente = head;
    int desplazamiento = 0;
    int n_copiados = 0;
    while(cliente){
        if(cliente->conectado == 1){
            if(n_copiados < (num_usu - 1) ){
                desplazamiento += sprintf(p_conectados + desplazamiento, "%s;", cliente->nombre);
                n_copiados++;
            }
            else{ // Es el ultimo conectado
                desplazamiento += sprintf(p_conectados + desplazamiento, "%s", cliente->nombre);
                n_copiados++;
                break;
            }
        }
        cliente = cliente->next;
    }

    // Devolvemos los datos que faltan
    *n_conectados = num_usu;

    // Imprimimos el mensaje de éxito
    printf("s> CONNECTEDUSERS %s OK\n", nombre);

    pthread_mutex_unlock(mutex_lista);
    return 0;
}