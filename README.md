# Práctica Final de Sistemas Distribuidos

## 1. Requisitos e Instalación de Dependencias

Es necesario disponer de `gcc`, `make` y las cabeceras de `tirpc`. Estas últimas pueden ser instaladas ejecutando en la terminal de Linux:
```bash
sudo apt-get install build-essential libc6-dev libtirpc-dev
```

También son necesarias para el servicio web y el correcto funcionamiento del cliente las librerías Zeep y Spyne:
```bash
pip install spyne zeep
```

## 2. Compilación

Para compilar la parte de servidor de la app, basta con ejecutar el siguiente comando:
```bash
make
```
Este comando automatiza, mediante un archivo Makefile, la generación del ejecutable del servidor principal de la app (server) y la generación del ejecutable del servidor rpc de registro (registro_server).

Para realizar una limpieza de los archivos temporales y binarios:
```bash
make clean
```

## 3. Despliegue y Ejecución

Para el correcto funcionamiento del sistema, los procesos deben lanzarse en terminales diferentes:

### Paso 1. Conversor de Mensajes (Servicio Web):

Ejecutar en una terminal el siguiente comando:
```bash
python3 ws-conversor-service.py
```

### Paso 2. Servidor de Registro (RPC):

Ejecutar en otra terminal el siguiente comando:
```bash
./registro_server
```

### Paso 3. Servidor Principal:

Antes de ejecutarlo, es obligatorio definir la IP del servidor RPC mediante la variable de entorno LOG_RPC_IP:
```bash
LOG_RPC_IP=<ip> ./server -p <puerto>
```
Donde <puerto> es el puerto desde el que se desea ejecutar el servidor principal.

### Paso 4. Cliente Python:

Por cada cliente que se desee conectar, hay que ejecutar en una terminal:
```bash
python3 client.py -s <ip> -p <puerto>
```
Donde <ip> es la dirección IP del servidor principal y <puerto> es su puerto de escucha.
