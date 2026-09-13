#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

struct Game
{
    char tablero[3][3];
    int iniciado;
    /* 1 Corresponde a partida iniciada*/
};

struct Game *games;

void vaciar_tablero(struct Game *game)
{
    for (int x = 0; x < 3; x++)
    {
        for (int y = 0; y < 3; y++)
        {
            game->tablero[x][y] = ' ';
        }
    }
}

void mostrar_tablero(struct Game *game)
{
    for (int x = 0; x < 3; x++)
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
void main(int argc, char const *argv[])
{
    games = malloc(5 * sizeof(*games));
    /*Asignamos la memoria para los 5 juegos*/

    shutdown_server();
}
