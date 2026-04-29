#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "gestion.h"
#include <stdlib.h>

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