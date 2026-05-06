# Compilador a utilizar
CC = gcc

# Opciones de compilación: 
CFLAGS = -Wall -Wextra -g -pthread

# Nombre del ejecutable final
TARGET = server

# Archivos objeto requeridos
OBJS = servidor.o gestion.o mensajes.o

# Cabeceras (Para que Make sepa que si cambian, hay que recompilar)
HEADERS = almacenamiento.h gestion.h mensajes.h

# Regla principal
all: $(TARGET)

# Regla para enlazar el ejecutable final
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

# Regla genérica para compilar archivos .c a .o
# Al poner $(HEADERS) aquí, le decimos a make que todos los .o dependen de los .h
%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

# Regla para limpiar los archivos generados
clean:
	rm -f *.o $(TARGET)