# Compilador y opciones
CC = gcc
CFLAGS = -Wall -Wextra -g -pthread -I/usr/include/tirpc
LDLIBS = -lnsl -lpthread -ldl -ltirpc

# Nombres de los ejecutables finales
TARGET_CHAT = server
TARGET_RPC = registro_server

# Archivos generados por rpcgen
RPC_STUBS = registro_clnt.c registro_svc.c registro_xdr.c
RPC_HDR = registro.h

# Objetos separados para cada programa
# El chat es el cliente RPC
OBJS_CHAT = servidor.o gestion.o mensajes.o registro_clnt.o registro_xdr.o
# El registro es el servidor RPC
OBJS_RPC = registro_server.o registro_svc.o registro_xdr.o

# Cabeceras locales
HEADERS = almacenamiento.h gestion.h mensajes.h $(RPC_HDR)

# --- REGLAS ---

all: $(TARGET_CHAT) $(TARGET_RPC)

# 1. Enlazar el servidor de chat
$(TARGET_CHAT): $(OBJS_CHAT)
	$(CC) $(CFLAGS) -o $(TARGET_CHAT) $(OBJS_CHAT) $(LDLIBS)

# 2. Enlazar el servidor de logs (RPC)
$(TARGET_RPC): $(OBJS_RPC)
	$(CC) $(CFLAGS) -o $(TARGET_RPC) $(OBJS_RPC) $(LDLIBS)

# 3. Generar los archivos RPC a partir del .x
$(RPC_HDR) $(RPC_STUBS): registro.x
	rpcgen -N -M registro.x

# 4. Regla genérica para compilar archivos .c a .o
%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

# 5. Limpieza total: borra objetos, los dos ejecutables y los stubs
clean:
	rm -f *.o $(TARGET_CHAT) $(TARGET_RPC) $(RPC_HDR) $(RPC_STUBS)