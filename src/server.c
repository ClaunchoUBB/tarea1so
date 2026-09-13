#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <strings.h>

/*
struct sockaddr_in
{
    short int sin_family;      AF_INET
    unsigned short sin_port;   Número de puerto
    struct in_addr sin_addr;   Dirección IP
    unsigned char sin_zero[8]; Relleno con 0
};
*/

struct Game
{
    /*
    Estructura básica
    para la gestión de las partidas
    la idea es que cada partida tiene su propio tablero
    y un par de sockets para la comunicación con los jugadores
    */
    char tablero[3][3];
    /* 1 Corresponde a partida iniciada*/
};

struct Game *games; // Creamos la estructura global para los juegos

void vaciar_tablero(struct Game *game)
{
    int x;
    int y;
    /* Esta función existe para que a la hora de crear un juego, el tablero siempre esté lleno de ' ' */
    for (x = 0; x < 3; x++)
    {
        for (y = 0; y < 3; y++)
        {
            game->tablero[x][y] = ' ';
        }
    }
}

void mostrar_tablero(struct Game *game)
{
    int x;
    for (x = 0; x < 3; x++)
    {
        printf(" %c | %c | %c \n",
               game->tablero[x][0],
               game->tablero[x][1],
               game->tablero[x][2]);

        if (x < 2)
            printf("---+---+---\n");
    }
}

void shutdown_server()
{
    free(games);
}

int main(int argc, char *argv[])
{
    struct sockaddr_in direccion_propia;
    direccion_propia.sin_family = AF_INET;
    direccion_propia.sin_port = 0;
    direccion_propia.sin_addr.s_addr = htonl(INADDR_ANY);
    bzero(&(direccion_propia.sin_zero), 8);

    socklen_t size_direccion_propia = sizeof(direccion_propia);

    int socketfd;
    socketfd = socket(AF_INET, SOCK_STREAM, 0);
    /* Creamos el fichero descriptor del socket*/

    bind(socketfd, (struct sockaddr *)(&direccion_propia), size_direccion_propia);
    /* Enlazamos el fichero descriptor del socket con la socket adress de entrada*/

    getsockname(socketfd, (struct sockaddr *)&direccion_propia, &size_direccion_propia);
    /* Le pedimos el nombre para que el usuario conozca el puerto */

    printf("Esperando en todas las interfaces de red\nPuerto:%d \n", direccion_propia.sin_port);
    /* Informamos al usuario */

    games = malloc(5 * sizeof(*games));
    /*Asignamos la memoria para los 5 juegos*/

    shutdown_server();
    /* Aquí matamos todos los sockets */

    return 0;
}
