from enum import Enum
import argparse
import socket
import threading

class client :

    # ******************** TYPES *********************
    # *
    # * @brief Return codes for the protocol methods
    class RC(Enum) :
        OK = 0
        ERROR = 1
        USER_ERROR = 2

    # ****************** ATTRIBUTES ******************
    _server = None
    _port = -1
    _nombre = None  # Sirve para recordar el nombre del usuario actual
    _listen_sock = None  # < Para poder cerrar el hilo después

    # Funcion auxiliar para leer toda la cadena hasta encontrar un \0
    @staticmethod
    def leer_cadena(socket_conexion):
        buffer = b''
        while True:
            byte = socket_conexion.recv(1)
            if byte == b'\0':
                break
            buffer += byte
            
        return buffer.decode('utf-8')

    # ******************** METHODS *******************
    # *
    # * @param user - User name to register in the system
    # * 
    # * @return OK if successful
    # * @return USER_ERROR if the user is already registered
    # * @return ERROR if another error occurred
    @staticmethod
    def  register(user) :
        try:
            # Se conecta al servidor
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.connect((client._server, client._port))

            # Se envía la cadena "REGISTER" indicando la operación
            op = "REGISTER\0"
            sock.sendall(op.encode('utf-8'))

            # Se envía el nombre del usuario
            nombre_usuario = f"{user}\0"
            sock.sendall(nombre_usuario.encode('utf-8'))

            # Se recibe el resultado (un byte) 
            resultado = sock.recv(1)

            # Cierra la conexión
            sock.close()

            # Comprobamos el resultado
            if not resultado:
                print("c> REGISTER FAIL\n")
                return client.RC.ERROR
            
            resultado = resultado[0]
            
            match resultado:
                case 0:
                    print("c> REGISTER OK\n")
                    return client.RC.OK
                case 1:
                    print("c> USERNAME IN USE\n")
                    return client.RC.USER_ERROR
                case _:
                    print("c> REGISTER FAIL\n")
                    return client.RC.ERROR
                
        except Exception as e:
            print("c> REGISTER FAIL\n")
            return client.RC.ERROR

    # *
    # 	 * @param user - User name to unregister from the system
    # 	 * 
    # 	 * @return OK if successful
    # 	 * @return USER_ERROR if the user does not exist
    # 	 * @return ERROR if another error occurred
    @staticmethod
    def  unregister(user) :
        try:
            # Se conecta al servidor
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.connect((client._server, client._port))

            # Se envía la cadena "UNREGISTER" indicando la operación
            op = "UNREGISTER\0"
            sock.sendall(op.encode('utf-8'))

            # Se envía el nombre del usuario
            nombre_usuario = f"{user}\0"
            sock.sendall(nombre_usuario.encode('utf-8'))

            # Se recibe el resultado (un byte) 
            resultado = sock.recv(1)

            # Cierra la conexión
            sock.close()

            # Comprobamos el resultado
            if not resultado:
                print("c> UNREGISTER FAIL\n")
                return client.RC.ERROR
            
            resultado = resultado[0]
            
            match resultado:
                case 0:
                    print("c> UNREGISTER OK\n")
                    return client.RC.OK
                case 1:
                    print("c> USER DOES NOT EXIST\n")
                    return client.RC.USER_ERROR
                case _:
                    print("c> UNREGISTER FAIL\n")
                    return client.RC.ERROR
                
        except Exception as e:
            print("c> UNREGISTER FAIL\n")
            return client.RC.ERROR
        
    @staticmethod
    def hilo_escucha(listen_sock):
        """
        Este hilo se ejecutará en segundo plano. 
        Su trabajo es aceptar conexiones del servidor y recibir mensajes.
        """
        while True:
            try:
                # Esperamos a que el servidor se conecte a nosotros para darnos un mensaje
                conn, addr = listen_sock.accept()
                
                # ¡Aquí usaremos client.leer_cadena() más adelante para leer el mensaje!
                # De momento lo cerramos para que no se cuelgue.
                conn.close()
            except Exception as e:
                break


    # *
    # * @param user - User name to connect to the system
    # * 
    # * @return OK if successful
    # * @return USER_ERROR if the user does not exist or if it is already connected
    # * @return ERROR if another error occurred
    @staticmethod
    def  connect(user) :
        try:
            # Creamos un socket de escucha
            client.listen_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            # Al poner el puerto a 0, el Sistema Operativo nos asigna uno libre automáticamente
            client.listen_sock.bind(('0.0.0.0', 0))
            # Obtenemos cuál es ese puerto que nos han asignado
            mi_puerto_escucha = client.listen_sock.getsockname()[1]
            # Ponemos el socket en modo "escucha"
            client.listen_sock.listen(5)

            # Creamos el hilo pasándole nuestro socket de escucha
            hilo = threading.Thread(target=client.hilo_escucha, args=(client.listen_sock,))
            # daemon=True hace que el hilo se cierre automáticamente si salimos del programa
            hilo.daemon = True 
            hilo.start()

            # Se conecta al servidor
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.connect((client._server, client._port))

            # Se envía la cadena "CONNECT" indicando la operación
            op = "CONNECT\0"
            sock.sendall(op.encode('utf-8'))

            # Se envía el nombre del usuario
            nombre_usuario = f"{user}\0"
            sock.sendall(nombre_usuario.encode('utf-8'))

            # Le mandamos al servidor el puerto
            puerto_str = f"{mi_puerto_escucha}\0"
            sock.sendall(puerto_str.encode('utf-8')) 

            # Se recibe el resultado (un byte) 
            resultado = sock.recv(1)

            # Cierra la conexión
            sock.close()

            # Comprobamos el resultado
            if not resultado:
                print("c> CONNECT FAIL\n")
                return client.RC.ERROR
            
            resultado = resultado[0]
            
            match resultado:
                case 0:
                    # Guardamos quién somos para que USERS funcione después
                    client._nombre = user
                    print("c> CONNECT OK\n")
                    return client.RC.OK
                case 1:
                    print("c> CONNECT FAIL, USER DOES NOT EXIST\n")
                    return client.RC.USER_ERROR
                case 2:
                    print("c> USER ALREADY CONNECTED\n")
                    return client.RC.USER_ERROR
                case _:
                    print("c> CONNECT FAIL\n")
                    return client.RC.ERROR
                
        except Exception as e:
            print("c> CONNECT FAIL\n")
            return client.RC.ERROR


    # *
    # * 
    # * @return OK if successful
    # * @return USER_ERROR if the user does not exist or if it is already connected
    # * @return ERROR if another error occurred
    @staticmethod
    def  users() :
        # Comprobamos que este conectado ya que sino client._nombre será None y fallará
        if client._nombre is None:
            print("c> CONNECTED USERS FAIL, USER IS NOT CONNECTED\n")
            return client.RC.USER_ERROR
        
        try:
            # Se conecta al servidor
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.connect((client._server, client._port))

            # Se envía la cadena "USERS" indicando la operación
            op = "USERS\0"
            sock.sendall(op.encode('utf-8'))

            # Se envía el nombre del usuario
            nombre_usuario = f"{client._nombre}\0"
            sock.sendall(nombre_usuario.encode('utf-8'))

            # Se recibe el resultado (un byte) 
            resultado = sock.recv(1)

            # Comprobamos el resultado
            if not resultado:
                print("c> CONNECTED USERS FAIL\n")
                sock.close()
                return client.RC.ERROR
            
            resultado = resultado[0]
            
            match resultado:
                case 0:
                    # Leemos el numero de clientes conectados y lo imprimimos
                    num_str = client.leer_cadena(sock)
                    num_users = int(num_str)
                    print(f"c> CONNECTED USERS ({num_users} users connected) OK")

                    # Leemos el nombre de cada cliente conectado y los vamos imprimiendo
                    for i in range(num_users):
                        cliente = client.leer_cadena(sock)
                        print(f"{cliente}")

                    return client.RC.OK
                case 1:
                    print("c> CONNECTED USERS FAIL, USER IS NOT CONNECTED\n")
                    return client.RC.USER_ERROR
                case _:
                    print("c> CONNECTED USERS FAIL\n")
                    return client.RC.ERROR
            
            # Cierra la conexión
            sock.close()
                
        except Exception as e:
            print("c> CONNECTED USERS FAIL\n")
            return client.RC.ERROR



    # *
    # * @param user - User name to disconnect from the system
    # * 
    # * @return OK if successful
    # * @return USER_ERROR if the user does not exist
    # * @return ERROR if another error occurred
    @staticmethod
    def  disconnect(user) :
        try:
            # Se conecta al servidor
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.connect((client._server, client._port))

            # Se envía la cadena "DISCONNECT" indicando la operación
            op = "DISCONNECT\0"
            sock.sendall(op.encode('utf-8'))

            # Se envía el nombre del usuario
            nombre_usuario = f"{user}\0"
            sock.sendall(nombre_usuario.encode('utf-8'))

            # Se recibe el resultado (un byte) 
            resultado = sock.recv(1)

            # Cierra la conexión
            sock.close()

            # Se debe parar siempre
            if client._listen_sock is not None:
                client._listen_sock.close() # Esto destraba el .accept() del hilo y lo cierra
                client._listen_sock = None
                
            client._nombre = None # Limpiamos el usuario actual

            # Comprobamos el resultado
            if not resultado:
                print("c> DISCONNECT FAIL\n")
                return client.RC.ERROR
            
            resultado = resultado[0]
            
            match resultado:
                case 0:
                    print("c> DISCONNECT OK\n")
                    return client.RC.OK
                case 1:
                    print("c> DISCONNECT FAIL, USER DOES NOT EXIST")
                    return client.RC.USER_ERROR
                case 2:
                    print("c> DISCONNECT FAIL, USER NOT CONNECTED")
                    return client.RC.USER_ERROR
                case _:
                    print("c> DISCONNECT FAIL")
                    return client.RC.ERROR
                
        except Exception as e:
            if client._listen_sock is not None:
                client._listen_sock.close()
                client._listen_sock = None
            client._nombre = None
            print("c> DISCONNECT FAIL")
            return client.RC.ERROR

    # *
    # * @param user    - Receiver user name
    # * @param message - Message to be sent
    # * 
    # * @return OK if the server had successfully delivered the message
    # * @return USER_ERROR if the user is not connected (the message is queued for delivery)
    # * @return ERROR the user does not exist or another error occurred
    @staticmethod
    def  send(user,  message) :
        #  Write your code here
        return client.RC.ERROR

    # *
    # * @param user    - Receiver user name
    # * @param file    - file  to be sent
    # * @param message - Message to be sent
    # * 
    # * @return OK if the server had successfully delivered the message
    # * @return USER_ERROR if the user is not connected (the message is queued for delivery)
    # * @return ERROR the user does not exist or another error occurred
    @staticmethod
    def  sendAttach(user,  file,  message) :
        #  Write your code here
        return client.RC.ERROR

    # *
    # **
    # * @brief Command interpreter for the client. It calls the protocol functions.
    @staticmethod
    def shell():

        while (True) :
            try :
                command = input("c> ")
                line = command.split(" ")
                if (len(line) > 0):

                    line[0] = line[0].upper()

                    if (line[0]=="REGISTER") :
                        if (len(line) == 2) :
                            client.register(line[1])
                        else :
                            print("Syntax error. Usage: REGISTER <userName>")

                    elif(line[0]=="UNREGISTER") :
                        if (len(line) == 2) :
                            client.unregister(line[1])
                        else :
                            print("Syntax error. Usage: UNREGISTER <userName>")

                    elif(line[0]=="CONNECT") :
                        if (len(line) == 2) :
                            client.connect(line[1])
                        else :
                            print("Syntax error. Usage: CONNECT <userName>")

                    elif(line[0]=="DISCONNECT") :
                        if (len(line) == 2) :
                            client.disconnect(line[1])
                        else :
                            print("Syntax error. Usage: DISCONNECT <userName>")

                    elif(line[0]=="USERS") :
                        if (len(line) == 1) :
                            client.users()
                        else :
                            print("Syntax error. Usage: CONNECTED_USERS <userName>")

                    elif(line[0]=="SEND") :
                        if (len(line) >= 3) :
                            #  Remove first two words
                            message = ' '.join(line[2:])
                            client.send(line[1], message)
                        else :
                            print("Syntax error. Usage: SEND <userName> <message>")

                    elif(line[0]=="SENDATTACH") :
                        if (len(line) >= 4) :
                            #  Remove first two words
                            message = ' '.join(line[3:])
                            client.sendAttach(line[1], line[2], message)
                        else :
                            print("Syntax error. Usage: SENDATTACH <userName> <filename> <message>")

                    elif(line[0]=="QUIT") :
                        if (len(line) == 1) :
                            break
                        else :
                            print("Syntax error. Use: QUIT")
                    else :
                        print("Error: command " + line[0] + " not valid.")
            except Exception as e:
                print("Exception: " + str(e))

    # *
    # * @brief Prints program usage
    @staticmethod
    def usage() :
        print("Usage: python3 client.py -s <server> -p <port>")


    # *
    # * @brief Parses program execution arguments
    @staticmethod
    def  parseArguments(argv) :
        parser = argparse.ArgumentParser()
        parser.add_argument('-s', type=str, required=True, help='Server IP')
        parser.add_argument('-p', type=int, required=True, help='Server Port')
        args = parser.parse_args()

        if (args.s is None):
            parser.error("Usage: python3 client.py -s <server> -p <port>")
            return False

        if ((args.p < 1024) or (args.p > 65535)):
            parser.error("Error: Port must be in the range 1024 <= port <= 65535");
            return False;
        
        client._server = args.s
        client._port = args.p

        return True


    # ******************** MAIN *********************
    @staticmethod
    def main(argv) :
        if (not client.parseArguments(argv)) :
            client.usage()
            return

        #  Write code here
        client.shell()
        print("+++ FINISHED +++")
    

if __name__=="__main__":
    client.main([])
