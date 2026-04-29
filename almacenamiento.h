#define MAX_NAME 256
#define MAX_MSG 256
#define MAX_IP 16

// Para guardar los mensajes pendientes de entregar a cada persona
typedef struct Nodo_mensaje {
    unsigned int id;                  // Identificador numérico del mensaje
    char remitente[MAX_NAME];         // Usuario que envía el mensaje originalmente
    char contenido[MAX_MSG];          // Texto del mensaje (máximo 256 bytes con '\0')
    
    struct Nodo_mensaje *next;        // Puntero al siguiente mensaje en la cola
} nodo_mensaje;


// Para guardar los clientes registrados y sus datos
typedef struct Nodo_clientes {
    char nombre[MAX_NAME];  // Nombre del cliente con el que se registró
    int conectado;          // 0 si no esta conectado, 1 en caso contrario

    // Campos de red (solo tienen sentido cuando 'conectado' es 1)
    char ip[MAX_IP];                  // IP del cliente
    int puerto;                       // Puerto 

    unsigned int ultimo_id; // Ultimo ID usado para este usuario

    // Puntero a la cabeza de la lista de mensajes sin entregar a esta persona
    nodo_mensaje *mensajes_pendientes;

    struct Nodo_clientes *next; // Puntero al siguiente cliente registrado
    struct Nodo_clientes *before; // Puntero al anterior cliente registrado
} nodo_clientes;

