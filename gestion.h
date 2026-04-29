#ifndef GESTION_H
#define GESTION_H

#define OP_REGISTER    0
#define OP_UNREGISTER  1
#define OP_CONNECT     2
#define OP_DISCONNECT  3
#define OP_SEND        4
#define OP_USERS       5
#define OP_SENDATTACH  6

int op_a_int(char *operacion);

int existe_usuario(char *nombre);

unsigned char registrar_usuario(char *nombre);

unsigned char dar_de_baja_usuario(char *nombre);

unsigned char conectar_usuario(char *nombre, int puerto_int, char *ip_cliente);

#endif