#ifndef MENSAJES_H
#define MENSAJES_H

int sendMessage(int socket, char * buffer, int len);

ssize_t readLine(int fd, void *buffer, size_t n);

#endif