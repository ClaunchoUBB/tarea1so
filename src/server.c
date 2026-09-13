#include <stdlib.h>
#include <unistd.h>
#include <string.h>

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
            game->tablero[x][y] = 0;
        }
    }
}

void shutdown_server(){
    free(games);
}
void main(int argc, char const *argv[])
{
    games = malloc(5 * sizeof(*games));
    /*Asignamos la memoria para los 5 juegos*/




    shutdown_server();
}

