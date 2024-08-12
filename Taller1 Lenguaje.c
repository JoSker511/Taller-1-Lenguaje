#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAX 1000

int *mark;           // Array para marcar los estados de aceptación
int **mat;           // Matriz de transiciones
int *symdir;         // Mapa para convertir símbolos a índices
int estado_inicial;  // Estado inicial
int num_estados;     // Número de estados
int num_simbolos;    // Número de símbolos en el alfabeto

char **estados;      // Array para almacenar los nombres de los estados
char *symbols;       // Array para almacenar el alfabeto

FILE *archivo = NULL;

void init(int estados_count, int simbolos_count) {
    num_estados = estados_count;
    num_simbolos = simbolos_count;

    mark = (int*)calloc(estados_count, sizeof(int));          // Inicializar los estados de aceptación
    symdir = (int*)malloc(128 * sizeof(int));                 // Inicializar el mapa de símbolos (ASCII)
    symbols = (char*)malloc(simbolos_count * sizeof(char));   // Inicializar el array de símbolos

    estados = (char**)malloc(estados_count * sizeof(char*));  // Inicializar el array de nombres de estados
    for (int i = 0; i < estados_count; i++) {
        estados[i] = (char*)malloc(10 * sizeof(char));        // Asignar memoria para cada nombre de estado
    }

    mat = (int**)malloc(estados_count * sizeof(int*));        // Inicializar la matriz de transiciones
    for (int i = 0; i < estados_count; i++) {
        mat[i] = (int*)malloc(128 * sizeof(int));
        for (int j = 0; j < 128; j++) {
            mat[i][j] = -1;  // Inicializar todas las transiciones a -1
        }
    }

    // Inicializar el mapa de símbolos a -1 para valores que no están en el alfabeto
    for (int i = 0; i < 128; i++) {
        symdir[i] = -1;
    }
}

// Función para buscar el índice de un estado en el array de estados
int obtenerIndiceEstado(const char* estado) {
    for (int i = 0; i < num_estados; i++) {
        if (strcmp(estados[i], estado) == 0) {
            return i;
        }
    }
    printf("Error: estado %s no encontrado.\n", estado);
    exit(1);
    return -1; // Solo para evitar advertencias, nunca debería llegar aquí
}

// Función para contar el número de estados y símbolos en el alfabeto
void contarEstadosYSimbolos(FILE *archivo) {
    char linea[MAX];
    int estado_count = 0;
    int simbolo_count = 0;

    // Leer los estados (primera línea)
    fgets(linea, MAX, archivo);
    char *token = strtok(linea, ",");
    while (token != NULL) {
        estado_count++;
        token = strtok(NULL, ",");
    }
    num_estados = estado_count;

    // Leer el alfabeto (segunda línea)
    fgets(linea, MAX, archivo);
    token = strtok(linea, ",");
    while (token != NULL) {
        simbolo_count++;
        token = strtok(NULL, ",");
    }
    num_simbolos = simbolo_count;
}

// Restablecer el puntero de archivo después de contar estados y símbolos
void resetArchivo(FILE *archivo) {
    rewind(archivo);
}

void leerEstados() {
    printf("Leyendo conjunto de estados...\n");
    char linea[MAX];
    if (fgets(linea, MAX, archivo) != NULL) {
        char *token = strtok(linea, ",");
        int i = 0;
        while (token != NULL && i < num_estados) {
            strcpy(estados[i], token);
            estados[i][strcspn(estados[i], "\n")] = '\0';  // Eliminar salto de línea si está presente
            i++;
            token = strtok(NULL, ",");
        }
    } else {
        printf("Error: No se pudo leer el conjunto de estados.\n");
        exit(1);
    }
}

void leerAlfabeto() {
    printf("Leyendo alfabeto...\n");
    char linea[MAX];
    if (fgets(linea, MAX, archivo) != NULL) {
        strcpy(symbols, linea);
        int n = strlen(symbols);
        if (symbols[n-1] == '\n') symbols[n-1] = '\0'; // Eliminar salto de línea si está presente

        for (int i = 0; i < n; i++) {
            symdir[(int)symbols[i]] = i;
        }
    } else {
        printf("Error: No se pudo leer el alfabeto.\n");
        exit(1);
    }
}

void leerEstadoInicial() {
    printf("Leyendo estado inicial...\n");
    char linea[MAX];
    if (fgets(linea, MAX, archivo) != NULL) {
        linea[strcspn(linea, "\n")] = '\0'; // Eliminar salto de línea si está presente
        estado_inicial = obtenerIndiceEstado(linea);
    } else {
        printf("Error: No se pudo leer el estado inicial.\n");
        exit(1);
    }
}

void leerEstadosAceptacion() {
    printf("Leyendo estados de aceptación...\n");
    char linea[MAX];
    if (fgets(linea, MAX, archivo) != NULL) {
        char *token = strtok(linea, ",");
        while (token != NULL) {
            token[strcspn(token, "\n")] = '\0'; // Eliminar salto de línea si está presente
            int indice_estado = obtenerIndiceEstado(token);
            mark[indice_estado] = 1;
            token = strtok(NULL, ",");
        }
    } else {
        printf("Error: No se pudo leer los estados de aceptación.\n");
        exit(1);
    }
}

void leerTransiciones() {
    printf("Leyendo transiciones...\n");
    char linea[MAX];
    int hay_transiciones = 0; // Bandera para verificar si hay transiciones

    while (fgets(linea, MAX, archivo) != NULL) {
        hay_transiciones = 1;
        char estado_actual[10], estado_siguiente[10];
        char simbolo;

        int items = sscanf(linea, "%[^,],%c,%[^,\n]", estado_actual, &simbolo, estado_siguiente);
        if (items == 3) {
            int origen = obtenerIndiceEstado(estado_actual);
            int destino = obtenerIndiceEstado(estado_siguiente);

            if ((int)simbolo >= 0 && (int)simbolo < 128) {
                mat[origen][(int)simbolo] = destino;
            } else {
                printf("Error: símbolo fuera de rango en la línea: %s\n", linea);
                exit(1);
            }
        } else {
            printf("Error: línea de transición malformada. Línea: %s\n", linea);
            exit(1);
        }
    }

    if (!hay_transiciones) {
        printf("No se encontraron transiciones. Se utilizará el estado inicial como estado final.\n");
    }
}

void processArgs(int argc, char *argv[]) {
    char fileName[256];
    archivo = NULL;

    printf("Ingrese el nombre del archivo de configuración del autómata: ");
    fgets(fileName, sizeof(fileName), stdin);
    fileName[strcspn(fileName, "\n")] = '\0';
    printf("Intentando abrir el archivo: %s\n", fileName);

    // Intentar abrir el archivo
    if ((archivo = fopen(fileName, "rt")) == NULL) {
        printf("Error al abrir el archivo %s. Saliendo...\n", fileName);
        exit(1);
    }
}

int main(int argc, char *argv[]) {
    // Procesar los argumentos y pedir el archivo de configuración
    processArgs(argc, argv);

    // Contar los estados y símbolos antes de inicializar
    contarEstadosYSimbolos(archivo);
    resetArchivo(archivo); // Restablecer el puntero del archivo para empezar desde el principio

    // Inicializar las estructuras dinámicamente
    init(num_estados, num_simbolos);

    // Leer y configurar el autómata desde el archivo
    leerEstados();
    leerAlfabeto();
    leerEstadoInicial();
    leerEstadosAceptacion();
    leerTransiciones();

    //--------------------------------------------------------//

    char str1[MAX];
    printf("Ingrese una cadena para validar (o 'exit' para salir): ");
    while (fgets(str1, MAX, stdin) != NULL) {
        if (strcmp(str1, "exit\n") == 0) break;

        int curr = estado_inicial;
        int limit = strlen(str1) - 1;

        if (str1[limit] == '\n') {
            str1[limit] = '\0';
            limit--;
        }

        for (int i = 0; i < limit; i++) {
            if (strchr(symbols, str1[i]) == NULL) {
                printf("ERROR! En la cadena %s, %c no es un elemento del alfabeto\n", str1, str1[i]);
                curr = -1;
                break;
            }

            int ele = (int)str1[i];
            if (ele < 0 || ele >= 128) {
                printf("ERROR! Índice fuera de rango para el símbolo %c.\n", str1[i]);
                curr = -1;
                break;
            }

            curr = mat[curr][ele];
            if (curr == -1) {
                printf("ERROR! Regla inválida!\n");
                break;
            }
        }

        if (curr != -1) {
            printf("La cadena %s es ", str1);
            if (mark[curr] == 1)
                printf("aceptada\n");
            else
                printf("rechazada\n");
        }

        printf("Ingrese una cadena para validar (o 'exit' para salir): ");
    }

    // Liberar la memoria dinámica al final
    free(mark);
    free(symdir);
    free(symbols);
    for (int i = 0; i < num_estados; i++) {
        free(estados[i]);
        free(mat[i]);
    }
    free(estados);
    free(mat);

    if (archivo != NULL) fclose(archivo);

    return 0;
}
