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

    char tablero[9];
    /*
    El tablero será representado de la sigueinte manera

    [0][1][2]
    [3][4][5]  == [0][1][2][3][4][5][6][7][8]
    [6][7][8]

    */

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
            game->tablero[x] = ' ';
        }
    }
}

void shutdown_server()
{
    free(games);
}

int check_jugada(int jugada, char signo_jugador, struct Game *partida)
{
    if (jugada > 8 || jugada < 0)
    {
        return -1;
    }
    if (partida->tablero[jugada] != ' ')
    {
        // Si el espacio donde se quiere hacer una jugada es distinto de ' '
        // Se retorna -1, indicando una jugada inválida
        return -1;
    }
    // En caso de que la jugada sea válida se lleva a cabo
    // y verificamos si estaba en el centro
    partida->tablero[jugada] = signo_jugador;
    if (jugada == 4)
    {
        // De ser el caso, comprobamos las diagonales
        if ((partida->tablero[0] && partida->tablero[8]) == signo_jugador)
        {
            return 1; // 1 corresponde con una victoria del jugador en turno
        }
        if ((partida->tablero[6] && partida->tablero[2]) == signo_jugador)
        {
            return 1;
        }
    }
    else
    {
        // En caso de que no esté en el centro, solo comprobamos las posiciones en forma de +
        // para esto usaremos un switch que variará según el entero de la jugada

        switch (jugada)
        {
        case 0:

            // (partida->tablero[]&&partida->tablero[]) == signo_jugador

            if ((partida->tablero[1] && partida->tablero[2]) == signo_jugador)
            {
                return 1;
            }
            if ((partida->tablero[3] && partida->tablero[6]) == signo_jugador)
            {
                return 1;
            }

            return 0;
            break;

        case 1:
            if ((partida->tablero[0] && partida->tablero[2]) == signo_jugador)
            {
                return 1;
            }
            if ((partida->tablero[4] && partida->tablero[7]) == signo_jugador)
            {
                return 1;
            }

            return 0;
            break;
        case 2:
            if ((partida->tablero[0] && partida->tablero[1]) == signo_jugador)
            {
                return 1;
            }
            if ((partida->tablero[5] && partida->tablero[8]) == signo_jugador)
            {
                return 1;
            }

            return 0;
            break;
        case 3:
            if ((partida->tablero[0] && partida->tablero[6]) == signo_jugador)
            {
                return 1;
            }
            if ((partida->tablero[4] && partida->tablero[5]) == signo_jugador)
            {
                return 1;
            }

            return 0;
            break;
        case 4:
            if ((partida->tablero[1] && partida->tablero[7]) == signo_jugador)
            {
                return 1;
            }
            if ((partida->tablero[3] && partida->tablero[5]) == signo_jugador)
            {
                return 1;
            }

            return 0;
            break;
        case 5:
            if ((partida->tablero[2] && partida->tablero[8]) == signo_jugador)
            {
                return 1;
            }
            if ((partida->tablero[3] && partida->tablero[4]) == signo_jugador)
            {
                return 1;
            }

            return 0;
            break;
        case 6:
            if ((partida->tablero[0] && partida->tablero[3]) == signo_jugador)
            {
                return 1;
            }
            if ((partida->tablero[7] && partida->tablero[8]) == signo_jugador)
            {
                return 1;
            }

            return 0;
            break;
        case 7:
            if ((partida->tablero[1] && partida->tablero[4]) == signo_jugador)
            {
                return 1;
            }
            if ((partida->tablero[6] && partida->tablero[8]) == signo_jugador)
            {
                return 1;
            }

            return 0;
            break;
        case 8:
            if ((partida->tablero[2] && partida->tablero[5]) == signo_jugador)
            {
                return 1;
            }
            if ((partida->tablero[6] && partida->tablero[7]) == signo_jugador)
            // SIX SEVEEEEEEN
            {
                return 1;
            }

            return 0;
            break;
        default:
            perror("Jugada no verificada\n");
            break;
        }
    }
}

int ejecutar_partida(struct Game *partida)
{
    int jugada_buff;
    int partida_finalizada = -1;
    int status_jugada;
    int contador_ddos = 0;
    // Este contador evitará que una persona mantenga
    // el juego en su turno abusando de usar jugadas inválidas, tienen 3 oportunidades
    // para hacer una jugada válida, si la tercera no es válida
    // se termina la conexión y se le concede la victoria al otro jugador
    while (partida_finalizada == -1)
    {
        recv(partida->jugadores[0], &jugada_buff, sizeof(jugada_buff), NULL);
        status_jugada = check_jugada(jugada_buff, 'X', partida);
        while (status_jugada == -1)
        {
            if (contador_ddos == 3)
            {
                send(partida->jugadores[0], -3, sizeof(-3), NULL);
                close(partida->jugadores[0]);
                send(partida->jugadores[1], 10, sizeof(10), NULL);
                // El código 10 será el de victoria
                close(partida->jugadores[1]);
                nExitTask(1);
            }
            send(partida->jugadores[0], -1, sizeof(-1), NULL);
            contador_ddos++;
            recv(partida->jugadores[0], &jugada_buff, sizeof(jugada_buff), NULL);
            status_jugada = check_jugada(jugada_buff, 'X', partida);
            // La idea es que el cliente pueda recibir este -1 e indicar al jugador que su jugada fue inválida
        }

        switch (status_jugada)
        {
        case 1:
            send(partida->jugadores[0], 10, sizeof(10), NULL);
            close(partida->jugadores[0]);
            send(partida->jugadores[1], 5, sizeof(5), NULL);
            // 5 será el código para una derrota
            close(partida->jugadores[1]);
            partida_finalizada = 0;
            break;
        case 0:
            send(partida->jugadores[0], 0, sizeof(0), NULL);
            send(partida->jugadores[0], partida->tablero, sizeof(partida->tablero), NULL);
            send(partida->jugadores[1], partida->tablero, sizeof(partida->tablero), NULL);
            send(partida->jugadores[1], 1, sizeof(1), NULL);
            send(partida->jugadores[0], 2, sizeof(2), NULL);
            // El 1 es el código para indicar "Te toca jugar"
            // El 2 es el código para indicar "Te toca esperar"

        default:
            send(partida->jugadores[0], 200, sizeof(200), NULL);
            send(partida->jugadores[1], 200, sizeof(200), NULL);
            close(partida->jugadores[0]);
            close(partida->jugadores[1]);
            perror("Resultado de jugada no manejado");
            break;
        }

        contador_ddos = 0;
        recv(partida->jugadores[1], &jugada_buff, sizeof(jugada_buff), NULL);
        status_jugada = check_jugada(jugada_buff, 'O', partida);
        while (status_jugada == -1)
        {
            if (contador_ddos == 3)
            {
                send(partida->jugadores[1], -3, sizeof(-3), NULL);
                close(partida->jugadores[1]);
                send(partida->jugadores[0], 10, sizeof(10), NULL);
                // El código 10 será el de victoria
                close(partida->jugadores[0]);
                nExitTask(1);
            }
            send(partida->jugadores[1], -1, sizeof(-1), NULL);
            contador_ddos++;
            recv(partida->jugadores[1], &jugada_buff, sizeof(jugada_buff), NULL);
            status_jugada = check_jugada(jugada_buff, 'O', partida);
        }

        switch (status_jugada)
        {
        case 1:
            send(partida->jugadores[1], 10, sizeof(10), NULL);
            close(partida->jugadores[1]);
            send(partida->jugadores[0], 5, sizeof(5), NULL);
            // 5 será el código para una derrota
            close(partida->jugadores[0]);
            partida_finalizada = 1;
            break;
        case 0:
            send(partida->jugadores[1], 0, sizeof(0), NULL);
            send(partida->jugadores[1], partida->tablero, sizeof(partida->tablero), NULL);
            send(partida->jugadores[0], partida->tablero, sizeof(partida->tablero), NULL);
            send(partida->jugadores[0], 1, sizeof(1), NULL);
            send(partida->jugadores[1], 2, sizeof(2), NULL);
            break;
        default:
            send(partida->jugadores[1], 200, sizeof(200), NULL);
            send(partida->jugadores[0], 200, sizeof(200), NULL);
            close(partida->jugadores[1]);
            close(partida->jugadores[0]);
            perror("Resultado de jugada no manejado");
            break;
        }
    }

    vaciar_tablero(partida);
    partida->jugadores[0] = 0;
    partida->jugadores[1] = 0;
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

    while (partidas_activas < 6)
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
