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

// Función auxiliar para enviar el mensaje y su correspondiente ACK por red.
// Devuelve 0 si hubo éxito y -1 si falló la conexión al destinatario.
int entregar_mensaje(nodo_clientes *cliente_dest, nodo_mensaje *mensaje) {
    // Creamos un socket TCP para hablar con el hilo de escucha del cliente destino
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Error creando socket para enviar mensaje");
        return -1;
    }

    struct sockaddr_in client_addr;
    client_addr.sin_family = AF_INET;
    client_addr.sin_port = htons(cliente_dest->puerto);
    client_addr.sin_addr.s_addr = inet_addr(cliente_dest->ip);

    if (connect(sock, (struct sockaddr *)&client_addr, sizeof(client_addr)) < 0) {
        perror("Error conectando al hilo de escucha del cliente destino");
        close(sock);
        return -1;
    }

    // Enviamos el mensaje al destinatario siguiendo el protocolo
    char op[] = "SEND MESSAGE\0";
    sendMessage(sock, op, strlen(op) + 1);
    
    char remitente[MAX_NAME + 1];
    sprintf(remitente, "%s", mensaje->remitente);
    sendMessage(sock, remitente, strlen(remitente) + 1);

    char id_str[20];
    sprintf(id_str, "%u", mensaje->id);
    sendMessage(sock, id_str, strlen(id_str) + 1);

    char contenido[MAX_MSG + 1];
    sprintf(contenido, "%s", mensaje->contenido);
    sendMessage(sock, contenido, strlen(contenido) + 1);

    close(sock);

    // Mensaje de log del servidor
    printf("s> SEND MESSAGE %u FROM %s TO %s\n", mensaje->id, mensaje->remitente, cliente_dest->nombre);

    // Buscamos al usuario que redactó el mensaje originalmente para el ACK
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
                char op_ack[] = "SEND MESS_ACK\0";
                sendMessage(sock_ack, op_ack, strlen(op_ack) + 1);
                sendMessage(sock_ack, id_str, strlen(id_str) + 1);
            } else {
                // Si falla la conexión con el remitente, el protocolo asume que se ha desconectado
                nodo_remitente->conectado = 0;
                nodo_remitente->puerto = 0;
                memset(nodo_remitente->ip, 0, MAX_IP);
                perror("Error conectando al remitente para enviar ACK. Marcando como desconectado");
            }
            close(sock_ack);
        }
    }
    return 0;
}

unsigned char registrar_usuario(char *nombre){
    pthread_mutex_lock(&mutex_lista);

    /*Comprobamos que no haya otro usuario registrado con el mismo nombre*/
    nodo_clientes *cliente = existe_usuario(nombre);
    if (cliente != NULL){
        // Imprimimos el mensaje de error
        printf("s> REGISTER %s FAIL\n", nombre);
        pthread_mutex_unlock(&mutex_lista);
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

    pthread_mutex_unlock(&mutex_lista);
    return 0;
}

unsigned char dar_de_baja_usuario(char *nombre){
    pthread_mutex_lock(&mutex_lista);

    /*Comprobamos que el usuario este registrado*/
    nodo_clientes *cliente = existe_usuario(nombre);
    if (cliente == NULL){
        // Imprimimos el mensaje de error
        printf("s> UNREGISTER %s FAIL\n", nombre);
        pthread_mutex_unlock(&mutex_lista);
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

    pthread_mutex_unlock(&mutex_lista);
    return 0;
}

unsigned char conectar_usuario(char *nombre, int puerto_cliente, char *ip_cliente){

    pthread_mutex_lock(&mutex_lista);

    /*Comprobamos que el usuario este registrado y no conectado*/
    nodo_clientes *cliente = existe_usuario(nombre);
    if (cliente == NULL){
        // Imprimimos el mensaje de error
        printf("s> CONNECT %s FAIL\n", nombre);
        pthread_mutex_unlock(&mutex_lista);
        return 1;
    }
    if (cliente->conectado == 1){
        // Imprimimos el mensaje de error
        printf("s> CONNECT %s FAIL\n", nombre);
        pthread_mutex_unlock(&mutex_lista);
        return 2;
    }

    // Rellenamos sus datos
    cliente->puerto = puerto_cliente;
    strcpy(cliente->ip,ip_cliente);
    cliente->conectado = 1;

    // Imprimimos el mensaje de éxito
    printf("s> CONNECT %s OK\n", nombre);

    // Le mandamos todos los mensajes guardados usando la función auxiliar
    nodo_mensaje *mensaje = cliente->mensajes_pendientes;
    while(mensaje){
        if (entregar_mensaje(cliente, mensaje) < 0) {
            break; // Si falla la conexión de repente, paramos de enviar
        }
        
        nodo_mensaje *aux = mensaje;
        mensaje = mensaje->next;
        free(aux);
    }

    // Actualizamos la lista de mensajes del cliente. 
    // Si todo fue bien, será NULL. Si hubo error a medias, apuntará al primer mensaje no enviado.
    cliente->mensajes_pendientes = mensaje;

    pthread_mutex_unlock(&mutex_lista);
    return 0;
}

unsigned char desconectar_usuario(char *nombre, char *ip_cliente){

    pthread_mutex_lock(&mutex_lista);

    /*Comprobamos que el usuario este registrado, conectado y su ip sea la misma a la que se guardó*/
    nodo_clientes *cliente = existe_usuario(nombre);
    if (cliente == NULL){
        // Imprimimos el mensaje de error
        printf("s> DISCONNECT %s FAIL\n", nombre);
        pthread_mutex_unlock(&mutex_lista);
        return 1;
    }
    if (cliente->conectado == 0){
        // Imprimimos el mensaje de error
        printf("s> DISCONNECT %s FAIL\n", nombre);
        pthread_mutex_unlock(&mutex_lista);
        return 2;
    }
    if (strcmp(cliente->ip, ip_cliente) != 0){
        // Imprimimos el mensaje de error
        printf("s> DISCONNECT %s FAIL\n", nombre);
        pthread_mutex_unlock(&mutex_lista);
        return 3;
    }

    // Rellenamos sus datos
    cliente->conectado = 0;           
    cliente->puerto = 0;              
    memset(cliente->ip, 0, MAX_IP);  

    // Imprimimos el mensaje de éxito
    printf("s> DISCONNECT %s OK\n", nombre);

    pthread_mutex_unlock(&mutex_lista);
    return 0;
}

unsigned char users(char *nombre, int *n_conectados, char **p_conectados){
    pthread_mutex_lock(&mutex_lista);

    /*Comprobamos que el usuario este registrado y conectado*/
    nodo_clientes *cliente = existe_usuario(nombre);
    if (cliente == NULL){
        // Imprimimos el mensaje de error
        printf("s> CONNECTEDUSERS %s FAIL\n", nombre);
        pthread_mutex_unlock(&mutex_lista);
        return 2;
    }
    if (cliente->conectado == 0){
        // Imprimimos el mensaje de error
        printf("s> CONNECTEDUSERS %s FAIL\n", nombre);
        pthread_mutex_unlock(&mutex_lista);
        return 1;
    }

    // Miramos todos los usuarios conectados actualmente
    nodo_clientes *aux = head;
    int num_usu = 0;
    int espacio_necesario = 0;
    while(aux){
        if(aux->conectado == 1){
            num_usu++; 
            espacio_necesario += strlen(aux->nombre) + 1; // +1 para el separador y el /0 final
        }
        aux = aux->next;
    }

    // Reservamos memoria para esos clientes y guardamos sus nombres
    *p_conectados = (char *)malloc(sizeof(char) * espacio_necesario);
    if (p_conectados == NULL){
        // Imprimimos el mensaje de error
        printf("s> CONNECTEDUSERS %s FAIL\n", nombre);
        pthread_mutex_unlock(&mutex_lista);
        return 2;
    }

    aux = head;
    int desplazamiento = 0;
    int n_copiados = 0;
    while(aux){
        if(aux->conectado == 1){
            if(n_copiados < (num_usu - 1) ){
                desplazamiento += sprintf((*p_conectados) + desplazamiento, "%s;", aux->nombre);
                n_copiados++;
            }
            else{ // Es el ultimo conectado
                desplazamiento += sprintf((*p_conectados) + desplazamiento, "%s", aux->nombre);
                n_copiados++;
                break;
            }
        }
        aux = aux->next;
    }

    // Devolvemos los datos que faltan
    *n_conectados = num_usu;

    // Imprimimos el mensaje de éxito
    printf("s> CONNECTEDUSERS %s OK\n", nombre);

    pthread_mutex_unlock(&mutex_lista);
    return 0;
}

unsigned char enviar_mensaje(char *remitente, char *destinatario, char *contenido, unsigned int *id_asignado) {
    pthread_mutex_lock(&mutex_lista);

    nodo_clientes *nodo_remitente = existe_usuario(remitente);
    nodo_clientes *nodo_destinatario = existe_usuario(destinatario);

    // Si uno de los dos no existe, enviamos error 1 al cliente
    if (nodo_remitente == NULL || nodo_destinatario == NULL) {
        pthread_mutex_unlock(&mutex_lista);
        return 1;
    }

    // Gestionamos el identificador del mensaje
    nodo_remitente->ultimo_id += 1;
    if (nodo_remitente->ultimo_id == 0) { // Manejo de overflow
        nodo_remitente->ultimo_id = 1;
    }
    unsigned int id_nuevo = nodo_remitente->ultimo_id;
    *id_asignado = id_nuevo;

    // Creamos el nodo del mensaje
    nodo_mensaje *nuevo_msg = (nodo_mensaje *)malloc(sizeof(nodo_mensaje));
    if (nuevo_msg == NULL) {
        pthread_mutex_unlock(&mutex_lista);
        return 2;
    }
    nuevo_msg->id = id_nuevo;
    strcpy(nuevo_msg->remitente, remitente);
    strcpy(nuevo_msg->contenido, contenido);
    nuevo_msg->next = NULL;

    // Lo almacenamos en la lista de mensajes pendientes del destinatario
    if (nodo_destinatario->mensajes_pendientes == NULL) {
        nodo_destinatario->mensajes_pendientes = nuevo_msg;
    } else {
        nodo_mensaje *temp = nodo_destinatario->mensajes_pendientes;
        while(temp->next != NULL) {
            temp = temp->next;
        }
        temp->next = nuevo_msg;
    }

    // Si el destinatario está conectado, intentamos entregarlo
    if (nodo_destinatario->conectado == 1) {
        if (entregar_mensaje(nodo_destinatario, nuevo_msg) == 0) {
            // Se entregó con éxito, por lo que lo quitamos de la cola y lo liberamos
            if (nodo_destinatario->mensajes_pendientes == nuevo_msg) {
                nodo_destinatario->mensajes_pendientes = NULL;
            } else {
                nodo_mensaje *temp = nodo_destinatario->mensajes_pendientes;
                while(temp->next != nuevo_msg) {
                    temp = temp->next;
                }
                temp->next = NULL;
            }
            free(nuevo_msg);
        }
    } 
    else {
        // El destinatario está desconectado, el mensaje se queda almacenado
        printf("s> MESSAGE %u FROM %s TO %s STORED\n", id_nuevo, remitente, destinatario);
    }

    pthread_mutex_unlock(&mutex_lista);
    return 0; 
} 