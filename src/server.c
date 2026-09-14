#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <strings.h>
#include <nSystem.h>

/*
struct sockaddr_in
{
    short int sin_family;      AF_INET
    unsigned short sin_port;   Número de puerto
    struct in_addr sin_addr;   Dirección IP
    unsigned char sin_zero[8]; Relleno con 0
};
*/

int clientes_conectados = 0;
int partidas_activas = 0;
/*
Se usan variables globales para
que los subprocesos también puedan leerlas
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

    int jugadores[2]
};

struct Game *games; // Creamos la estructura global para los juegos

struct Cola
{
    int clientes_encolados[2];
    int front;
    int back;
};



void vaciar_tablero(struct Game *game)
{

    /* Esta función existe para que a la hora de crear un juego, el tablero siempre esté lleno de ' ' */
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

int ejecutar_partida(struct Game *partida)
{
}

void init_server(int argc, char const *argv[])
{
    struct sockaddr_in direccion_propia;
    direccion_propia.sin_family = AF_INET;
    direccion_propia.sin_port = 0;
    direccion_propia.sin_addr.s_addr = htonl(INADDR_ANY);
    bzero(&(direccion_propia.sin_zero), 8);

    socklen_t size_direccion_propia = sizeof(direccion_propia);

    int socket_server;
    socket_server = socket(AF_INET, SOCK_STREAM, 0);
    /* Creamos el fichero descriptor del socket*/

    bind(socket_server, (struct sockaddr *)(&direccion_propia), size_direccion_propia);
    /* Enlazamos el fichero descriptor del socket con la socket adress de entrada*/

    getsockname(socket_server, (struct sockaddr *)&direccion_propia, &size_direccion_propia);
    /* Le pedimos el nombre para que el usuario conozca el puerto */

    printf("Esperando en todas las interfaces de red\nPuerto:%d \n", direccion_propia.sin_port);
    /* Informamos al usuario */

    games = malloc(5 * sizeof(*games));
    /*Asignamos la memoria para los 5 juegos*/

    /* Ahora empezamos a escuchar para que lleguen los usuarios*/

    listen(socket_server, 10);

    int cliente_esperando = -1;

    /*
    Creamos una conexión en -1, de esta manera, cuando el accept()
    reciba una conexión real, actualizará el valor y se podrá trabajar de forma
    lógica
    */

    while (1)
    {

        int cliente_nuevo = accept(socket_server, NULL, NULL);
        /* Esperamos una conexión en el socket del server, pero no nos interesa quien se conecta */

        if (partidas_activas = 5)
        {
            
        }

        clientes_conectados++;
        if (cliente_esperando == -1)
        { /* Si no hay cliente esperando, este empieza a esperar.*/
            cliente_esperando = cliente_nuevo;
            send(cliente_esperando, "Esperando contrincante", sizeof("Esperando contrincante"), NULL);
            continue;
            /* Se usa un continue para saltarse lo demás y volver a esperar */
        }
        games[partidas_activas].jugadores[0] = cliente_esperando;
        games[partidas_activas].jugadores[1] = cliente_nuevo;
        partidas_activas++;
        cliente_esperando = -1;
        nEmitTask(ejecutar_partida(&games[partidas_activas]));
        /* Utilizando nSystem llamamos a un subproceso para facilitar esta parte */
    }

    shutdown_server();
    /* Aquí matamos todos los sockets */
}
