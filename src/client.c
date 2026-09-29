#include <netinet/in.h> 
#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 
#include <sys/socket.h> 
#include <unistd.h> 
#include <arpa/inet.h>

// Definimos la estructura del tablero exactamente igual que en el servidor
struct GameBuffer {
    char tablero[3][3];
    char mensaje[100];  // Un buffer útil para que el servidor le diga al cliente qué hacer
    int mi_turno;       // 1 si el cliente debe mover, 0 si debe esperar, -1 si terminó el juego
};

void mostrar_tablero_cliente(char tablero[3][3]) {
    printf("\n");
    for (int x = 0; x < 3; x++) { 
        printf(" %c | %c | %c \n", tablero[x][0], tablero[x][1], tablero[x][2]); 
        if (x < 2) printf("---+---+---\n"); 
    }
    printf("\n");
}

int main(int argc, char const *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Uso: %s <IP_Servidor> <Puerto_Servidor>\n", argv[0]);
        return 1;
    }

    int socketfd;
    struct sockaddr_in direccion_servidor;

    // Crear el socket TCP
    socketfd = socket(AF_INET, SOCK_STREAM, 0);
    if (socketfd < 0) {
        perror("Error al crear el socket");
        return 1;
    }

    // Configurar la dirección del servidor
    direccion_servidor.sin_family = AF_INET;
    direccion_servidor.sin_port = htons(atoi(argv[2])); // Convertimos el puerto a Network Byte Order
    
    // Convertimos la IP de texto a binario
    if (inet_pton(AF_INET, argv[1], &direccion_servidor.sin_addr) <= 0) {
        perror("IP no válida o no soportada");
        close(socketfd);
        return 1;
    }

    // Conectarse al servidor (aquí el cliente entra a la cola si está lleno)
    printf("Conectando al servidor de Tres en Raya en %s:%s...\n", argv[1], argv[2]);
    if (connect(socketfd, (struct sockaddr *)&direccion_servidor, sizeof(direccion_servidor)) < 0) {
        perror("Error en la conexión");
        close(socketfd);
        return 1;
    }

    printf("¡Conectado exitosamente! Esperando instrucciones del servidor...\n");

    struct GameBuffer estado;
    
    // Función para traducir coordenadas (fila, columna) a un índice lineal (0 al 8)
    int traducir_a_indice(int fila, int columna) {
    return fila * 3 + columna;
    }


    // Bucle principal de juego
    while (1) {
        // Recibir la información cruda desde el servidor
        int bytes_recibidos = recv(socketfd, &estado, sizeof(estado), 0);
        if (bytes_recibidos <= 0) {
            printf("El servidor cerró la conexión o hubo un error.\n");
            break;
        }

        // 1. Mostrar siempre el mensaje instructivo del servidor (ej: "Esperando oponente", "¡Tu turno!")
        printf("[Servidor]: %s\n", estado.mensaje);

        // 2. Dibujar el tablero actual en la pantalla del cliente
        mostrar_tablero_cliente(estado.tablero);

        // 3. Evaluar el estado del turno
        if (estado.mi_turno == 1) {
            // Es nuestro turno de jugar, pedimos las coordenadas por consola
            do {
                printf("Ingresa tu jugada (Fila y Columna de 0 a 2 separados por espacio, ej: 1 2): ");
                if (scanf("%d %d", &fila, &columna) != 2) {
                    // Limpiar el búfer de entrada en caso de que metan letras
                    while (getchar() != '\n');
                    fila = -1; columna = -1;
                }
            } while (fila < 0 || fila > 2 || columna < 0 || columna > 2);

            // Empaquetamos la jugada en las primeras posiciones del tablero o creas un struct de movimiento.
            // Para mantenerlo simple, mandamos las coordenadas directamente en un array de 2 enteros al servidor:
            int movimiento[2] = {fila, columna};
            send(socketfd, movimiento, sizeof(movimiento), 0);
            
            printf("Enviando movimiento [%d, %d] al servidor...\n", fila, columna);
        } 
        else if (estado.mi_turno == -1) {
            // El servidor nos avisa que el juego terminó (ganaste, perdiste o empate)
            printf("Partida finalizada. ¡Gracias por jugar!\n");
            break;
        }
        else {
            // mi_turno == 0, toca esperar al rival
            printf("Esperando el movimiento del oponente...\n");
        }
    }

    // Cerrar socket
    close(socketfd);
    return 0;
}