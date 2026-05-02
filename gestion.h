#ifndef GESTION_H
#define GESTION_H

#include "almacenamiento.h"

#define OP_REGISTER    0
#define OP_UNREGISTER  1
#define OP_CONNECT     2
#define OP_DISCONNECT  3
#define OP_SEND        4
#define OP_USERS       5
#define OP_SENDATTACH  6


int op_a_int(char *operacion);

nodo_clientes *existe_usuario(char *nombre);

int entregar_mensaje(nodo_clientes *cliente_dest, nodo_mensaje *mensaje);

unsigned char registrar_usuario(char *nombre);

unsigned char dar_de_baja_usuario(char *nombre);

unsigned char conectar_usuario(char *nombre, int puerto_cliente, char *ip_cliente);

unsigned char desconectar_usuario(char *nombre, char *ip_cliente);

unsigned char users(char *nombre, int *n_conectados, char **p_conectados);

unsigned char enviar_mensaje(char *remitente, char *destinatario, char *contenido, unsigned int *id_asignado);

#endif