# Compilador y opciones
CC = gcc
CFLAGS = -Wall -Wextra -g -pthread -I/usr/include/tirpc
LDLIBS = -lpthread -ldl -ltirpc

# Nombres de los ejecutables finales
TARGET_CHAT = server
TARGET_RPC = registro_server

# Objetos separados para cada programa
# El chat es el cliente RPC
OBJS_CHAT = servidor.o gestion.o mensajes.o registro_clnt.o registro_xdr.o
# El registro es el servidor RPC
OBJS_RPC = registro_server.o registro_svc.o registro_xdr.o

# Cabeceras locales
HEADERS = almacenamiento.h gestion.h mensajes.h registro.h

# --- REGLAS ---

all: $(TARGET_CHAT) $(TARGET_RPC)

# 1. Enlazar el servidor principal
$(TARGET_CHAT): $(OBJS_CHAT)
	$(CC) $(CFLAGS) -o $(TARGET_CHAT) $(OBJS_CHAT) $(LDLIBS)

# 2. Enlazar el servidor de registro (RPC)
$(TARGET_RPC): $(OBJS_RPC)
	$(CC) $(CFLAGS) -o $(TARGET_RPC) $(OBJS_RPC) $(LDLIBS)

# 3. Regla genérica para compilar archivos .c a .o
%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

# 4. Limpieza total: borra los objetos (.o) y los ejecutables
clean:
	rm -f *.o $(TARGET_CHAT) $(TARGET_RPC)