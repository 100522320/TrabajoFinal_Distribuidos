#include <sys/socket.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "mensajes.h"
#include <stdlib.h>

int sendMessage(int socket, char * buffer, int len)
{
	int r;
	int l = len;
		

	do {	
		r = write(socket, buffer, l);
		l = l -r;
		buffer = buffer + r;
	} while ((l>0) && (r>=0));
	
	if (r < 0)
		return (-1);   /* fail */
	else
		return(0);	/* full length has been sent */
}

int recvMessage(int socket, char *buffer, int len)
{
	int r;
	int l = len;
		

	do {	
		r = read(socket, buffer, l);
		l = l -r ;
		buffer = buffer + r;
	} while ((l>0) && (r>=0));
	
	if (r < 0)
		return (-1);   /* fallo */
	else
		return(0);	/* full length has been receive */
}

int leerDato(int fd, char *tipo, char *dest){
    int len = 0;
    char c, delimitador;

    if(strcmp(tipo, "int")==0 || strcmp(tipo, "vec")==0){
        delimitador = ';';
    } else if(strcmp(tipo, "str")==0){
        delimitador = '\0';
    } else{
        printf("Error: el tipo introducido no es válido\n");
        return -1;
    }

    while (len < MAX_MESS) {
        if (read(fd, &c, 1) != 1) {
            return -1;
        }
        
        if (c == delimitador) {
            dest[len] = '\0';
            return 0;
        }
        
        dest[len++] = c;
    }

    printf("Error: tamaño máximo de mensaje excedido\n");
    return -1;
}


