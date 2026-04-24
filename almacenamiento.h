#define MAX_NAME 256

typedef struct Nodo_registro {
    char[MAX_NAME] nombre;
    struct Nodo_registro *next;
} nodo_registro;