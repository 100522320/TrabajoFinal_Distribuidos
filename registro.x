/*Primero definimos el número de caracteres máximo que permitiremos para el nombre del usuario y del fichero*/
const MAX_NAME = 256;

/*Definimos la estructura de datos de string con la extensión máxima definida*/
typedef string rpc_string<MAX_NAME>;

/*Definimos el programa, cuyo único procedimiento será el de imprimir*/
program REGISTRO_PROG {
    version REGISTRO_VERS {
        void IMPRIMIR(rpc_string op, rpc_string usuario, rpc_string fichero) = 1;
    } = 1;
} = 0x20522125; /* Número de programa en el rango de usuario usando uno de nuestros nias (100522125) */