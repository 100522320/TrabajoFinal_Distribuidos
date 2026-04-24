#ifndef MENSAJES_H
#define MENSAJES_H

int sendMessage(int socket, char * buffer, int len);

int recvMessage(int socket, char *buffer, int len);

int leerDato(int fd, char *tipo, char *dest);

#endif