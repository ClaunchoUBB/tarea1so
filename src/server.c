#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <strings.h>
#include <nSystem.h>
#include <fifoqueues.h>

/*

#T1
#4 de Octubre 2026
#Claudio Rodríguez Parra
#Jesús Vivanco Zambrano
#Luciano Venegas Castro

*/

/*
struct sockaddr_in
{
    short int sin_family;      AF_INET
    unsigned short sin_port;   Número de puerto
    struct in_addr sin_addr;   Dirección IP
    unsigned char sin_zero[8]; Relleno con 0
};
*/

/*
Creamos los ints para los códigos, son variables dado que
los mensajes entre procesos trabajan con direcciones de memoria
*/

int ESPERANDO_RIVAL = 3;
int TU_TURNO = 1;
int ESPERA_TURNO = 2;
int JUGADA_ACEPTADA = 0;
int JUGADA_INVALIDA = -1;
int DERROTA = 5;
int VICTORIA = 10;
int DEMASIADOS_ERRORES = -3;
int ERROR_INTERNO = 200;
int EMPATE = 4;

const int combinaciones[8][3] = {
    {0, 1, 2},
    {3, 4, 5},
    {6, 7, 8},
    {0, 3, 6},
    {1, 4, 7},
    {2, 5, 8},
    {0, 4, 8},
    {2, 4, 6}};
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
    int game_id;
    int libre;
    char tablero[9];
    /*
    El tablero será representado de la sigueinte manera

    [0][1][2]
    [3][4][5]  == [0][1][2][3][4][5][6][7][8]
    [6][7][8]
    */

    int jugadas_realizadas;
    int jugadores[2];
};

struct Game *games; // Creamos la estructura global para los juegos

void vaciar_tablero(struct Game *game)
{
    /* Esta función existe para que a la hora de crear un juego, el tablero siempre esté lleno de ' ' */
    for (int x = 0; x < 9; x++)
    {
        game->tablero[x] = ' ';
    }
}

void shutdown_server(int socket_server)
{
    /*
     Cerramos los sockets de los jugadores que todavía
     estén asociados a una partida.
    */
    for (int i = 0; i < 5; i++)
    {
        if (games[i].libre == 0)
        {
            if (games[i].jugadores[0] != -1)
            {
                close(games[i].jugadores[0]);
            }

            if (games[i].jugadores[1] != -1)
            {
                close(games[i].jugadores[1]);
            }
        }
    }

    /*
     Liberamos la memoria reservada para las partidas.
    */
    free(games);

    /*
     Cerramos el socket principal del servidor.
    */
    close(socket_server);
}

int check_jugada(int jugada, char signo_jugador, struct Game *partida)
{
    // La jugada debe estar entre 0 y 8
    if (jugada < 0 || jugada > 8)
    {
        return -1;
    }

    // La casilla debe estar vacía
    if (partida->tablero[jugada] != ' ')
    {
        return -1;
    }

    // Realizamos la jugada
    partida->tablero[jugada] = signo_jugador;
    partida->jugadas_realizadas++;

    /*
     * Todas las combinaciones posibles para ganar:
     *
     * 0 1 2
     * 3 4 5
     * 6 7 8
     *
     * Filas:
     * 0-1-2
     * 3-4-5
     * 6-7-8
     *
     * Columnas:
     * 0-3-6
     * 1-4-7
     * 2-5-8
     *
     * Diagonales:
     * 0-4-8
     * 2-4-6
     */

    // Comprobamos las 8 combinaciones
    for (int i = 0; i < 8; i++)
    {
        if (partida->tablero[combinaciones[i][0]] == signo_jugador &&
            partida->tablero[combinaciones[i][1]] == signo_jugador &&
            partida->tablero[combinaciones[i][2]] == signo_jugador)
        {
            return 1; // Victoria
        }
    }

    if (partida->jugadas_realizadas < 9) // Si llegó a este punto, y no ganó comprobamos si aún no llegan al limite de jugadas
    {
        // Si no ha llegado al límite, significa que es una jugada válida, pero nadie gana aún
        return 0;
    }
    else
    {
        // En cambio, si llegó al límite y nadie ha ganado, es empate
        return 4;
    }
}
/*
int ejecutar_partida(struct Game *partida)
{
    vaciar_tablero(partida);
    int jugada_buffer;
    int partida_finalizada = -1;
    int status_jugada;
    partida->jugadas_realizadas = 0;
    int contador_ddos = 0;
    // Este contador evitará que una persona mantenga
    // el juego en su turno abusando de usar jugadas inválidas, tienen 3 oportunidades
    // para hacer una jugada válida, si la tercera no es válida
    // se termina la conexión y se le concede la victoria al otro jugador
    while (partida_finalizada == -1)
    {
        recv(partida->jugadores[0], &jugada_buffer, sizeof(jugada_buffer), 0);
        status_jugada = check_jugada(jugada_buffer, 'X', partida);
        while (status_jugada == -1)
        {
            if (contador_ddos == 3)
            {
                send(partida->jugadores[0], &DEMASIADOS_ERRORES, sizeof(DEMASIADOS_ERRORES), 0);
                close(partida->jugadores[0]);
                send(partida->jugadores[1], &VICTORIA, sizeof(VICTORIA), 0);
                // El código 10 será el de victoria
                close(partida->jugadores[1]);

                partida->jugadores[0] = -1;
                partida->jugadores[1] = -1;
                partida->libre = 1;

                nExitTask(1);
            }
            send(partida->jugadores[0], &JUGADA_INVALIDA, sizeof(JUGADA_INVALIDA), 0);
            contador_ddos++;
            recv(partida->jugadores[0], &jugada_buffer, sizeof(jugada_buffer), 0);
            status_jugada = check_jugada(jugada_buffer, 'X', partida);
            // La idea es que el cliente pueda recibir este -1 e indicar al jugador que su jugada fue inválida
        }

        switch (status_jugada)
        {
        case 1:
            send(partida->jugadores[0], &VICTORIA, sizeof(VICTORIA), 0);
            close(partida->jugadores[0]);
            send(partida->jugadores[1], &DERROTA, sizeof(DERROTA), 0);
            // 5 será el código para una derrota
            close(partida->jugadores[1]);
            partida_finalizada = 1;
            continue;
        case 0:
            send(partida->jugadores[0], &JUGADA_ACEPTADA, sizeof(JUGADA_ACEPTADA), 0);
            send(partida->jugadores[0], partida->tablero, sizeof(partida->tablero), 0);
            send(partida->jugadores[1], partida->tablero, sizeof(partida->tablero), 0);
            send(partida->jugadores[1], &TU_TURNO, sizeof(TU_TURNO), 0);
            send(partida->jugadores[0], &ESPERA_TURNO, sizeof(ESPERA_TURNO), 0);
            // El 1 es el código para indicar "Te toca jugar"
            // El 2 es el código para indicar "Te toca esperar"
            break;
        case 4:
            send(partida->jugadores[0], partida->tablero, sizeof(partida->tablero), 0);
            send(partida->jugadores[1], partida->tablero, sizeof(partida->tablero), 0);
            send(partida->jugadores[0], &EMPATE, sizeof(EMPATE), 0);
            send(partida->jugadores[1], &EMPATE, sizeof(EMPATE), 0);
            partida_finalizada = 1;
            continue;

        default:
            send(partida->jugadores[0], &ERROR_INTERNO, sizeof(ERROR_INTERNO), 0);
            send(partida->jugadores[1], &ERROR_INTERNO, sizeof(ERROR_INTERNO), 0);
            close(partida->jugadores[0]);
            close(partida->jugadores[1]);
            perror("Resultado de jugada no manejado");
            continue;
        }

        contador_ddos = 0;
        recv(partida->jugadores[1], &jugada_buffer, sizeof(jugada_buffer), 0);
        status_jugada = check_jugada(jugada_buffer, 'O', partida);
        while (status_jugada == -1)
        {
            if (contador_ddos == 3)
            {
                send(partida->jugadores[1], &DEMASIADOS_ERRORES, sizeof(DEMASIADOS_ERRORES), 0);
                close(partida->jugadores[1]);
                send(partida->jugadores[0], &VICTORIA, sizeof(VICTORIA), 0);
                // El código 10 será el de victoria
                close(partida->jugadores[0]);
                partida->jugadores[0] = -1;
                partida->jugadores[1] = -1;
                partida->libre = 1;
                nExitTask(1);
            }
            send(partida->jugadores[1], &JUGADA_INVALIDA, sizeof(JUGADA_INVALIDA), 0);
            contador_ddos++;
            recv(partida->jugadores[1], &jugada_buffer, sizeof(jugada_buffer), 0);
            status_jugada = check_jugada(jugada_buffer, 'O', partida);
        }

        switch (status_jugada)
        {
        case 1:
            send(partida->jugadores[1], &VICTORIA, sizeof(VICTORIA), 0);
            close(partida->jugadores[1]);
            send(partida->jugadores[0], &DERROTA, sizeof(DERROTA), 0);
            // 5 será el código para una derrota
            close(partida->jugadores[0]);
            partida_finalizada = 1;
            continue;
        case 0:
            send(partida->jugadores[1], &JUGADA_ACEPTADA, sizeof(JUGADA_ACEPTADA), 0);
            send(partida->jugadores[1], partida->tablero, sizeof(partida->tablero), 0);
            send(partida->jugadores[0], partida->tablero, sizeof(partida->tablero), 0);
            send(partida->jugadores[0], &TU_TURNO, sizeof(TU_TURNO), 0);
            send(partida->jugadores[1], &ESPERA_TURNO, sizeof(ESPERA_TURNO), 0);
            break;
        case 4:
            send(partida->jugadores[0], partida->tablero, sizeof(partida->tablero), 0);
            send(partida->jugadores[1], partida->tablero, sizeof(partida->tablero), 0);
            send(partida->jugadores[0], &EMPATE, sizeof(EMPATE), 0);
            send(partida->jugadores[1], &EMPATE, sizeof(EMPATE), 0);
            partida_finalizada = 1;
            continue;
        default:
            send(partida->jugadores[1], &ERROR_INTERNO, sizeof(ERROR_INTERNO), 0);
            send(partida->jugadores[0], &ERROR_INTERNO, sizeof(ERROR_INTERNO), 0);
            close(partida->jugadores[1]);
            close(partida->jugadores[0]);
            perror("Resultado de jugada no manejado");
            continue;
        }
    }
    vaciar_tablero(partida);
    partida->jugadores[0] = -1;
    partida->jugadores[1] = -1;
    partida->libre = 1;
}
*/
int jugar_partida(struct Game *partida)
{
    const char signos[2] = {'X', 'O'};
    int turno = 0;
    int jugada_buffer;
    int status_jugada;
    int contador_errores;

    vaciar_tablero(partida);
    partida->jugadas_realizadas = 0;

    for (;;)
    {
        int a = partida->jugadores[turno];     // juega
        int b = partida->jugadores[1 - turno]; // espera
        contador_errores = 0;

        recv(a, &jugada_buffer, sizeof(jugada_buffer), 0);
        status_jugada = check_jugada(jugada_buffer, signos[turno], partida);

        while (status_jugada == -1)
        {
            if (contador_errores == 3)
            {
                send(a, &DEMASIADOS_ERRORES, sizeof(DEMASIADOS_ERRORES), 0);
                close(a);
                send(b, &VICTORIA, sizeof(VICTORIA), 0);
                close(b);
                return 1;
            }
            send(a, &JUGADA_INVALIDA, sizeof(JUGADA_INVALIDA), 0);
            contador_errores++;
            recv(a, &jugada_buffer, sizeof(jugada_buffer), 0);
            status_jugada = check_jugada(jugada_buffer, signos[turno], partida);
        }

        switch (status_jugada)
        {
        case 1: // Victoria de a
            send(a, &VICTORIA, sizeof(VICTORIA), 0);
            send(b, &DERROTA, sizeof(DERROTA), 0);
            close(a);
            close(b);
            return 0;

        case 4: // Empate
            send(a, partida->tablero, sizeof(partida->tablero), 0);
            send(b, partida->tablero, sizeof(partida->tablero), 0);
            send(a, &EMPATE, sizeof(EMPATE), 0);
            send(b, &EMPATE, sizeof(EMPATE), 0);
            close(a);
            close(b);
            return 0;

        case 0: // Jugada válida, la partida sigue
            send(a, &JUGADA_ACEPTADA, sizeof(JUGADA_ACEPTADA), 0);
            send(a, partida->tablero, sizeof(partida->tablero), 0);
            send(b, partida->tablero, sizeof(partida->tablero), 0);
            send(b, &TU_TURNO, sizeof(TU_TURNO), 0);
            send(a, &ESPERA_TURNO, sizeof(ESPERA_TURNO), 0);
            break;

        default:
            send(a, &ERROR_INTERNO, sizeof(ERROR_INTERNO), 0);
            send(b, &ERROR_INTERNO, sizeof(ERROR_INTERNO), 0);
            close(a);
            close(b);
            perror("Resultado de jugada no manejado");
            return -1;
        }

        turno = 1 - turno; // cambia el turno
    }
}

int buscar_slot(struct Game *partidas)
{
    for (int i = 0; i < 5; i++)
    {
        if (partidas[i].libre == 1)
        {
            return i;
        }
    }
    return -1;
}

void init_server()
{
    struct sockaddr_in direccion_propia;
    direccion_propia.sin_family = AF_INET;
    direccion_propia.sin_port = 0;
    direccion_propia.sin_addr.s_addr = htonl(INADDR_ANY);
    bzero(&(direccion_propia.sin_zero), 8);
    FifoQueue en_espera = MakeFifoQueue();
    socklen_t size_direccion_propia = sizeof(direccion_propia);
    int siguiente_game_id = 1;
    int socket_server;
    socket_server = socket(AF_INET, SOCK_STREAM, 0);
    /* Creamos el fichero descriptor del socket*/

    bind(socket_server, (struct sockaddr *)(&direccion_propia), size_direccion_propia);
    /* Enlazamos el fichero descriptor del socket con la socket adress de entrada*/

    getsockname(socket_server, (struct sockaddr *)&direccion_propia, &size_direccion_propia);
    /* Le pedimos el nombre para que el usuario conozca el puerto */

    printf("Esperando en todas las interfaces de red\nPuerto:%d \n", ntohs(direccion_propia.sin_port));
    /* Informamos al usuario */

    /* Alojamos las partidas en la memoria */
    games = malloc(5 * sizeof(*games));

    for (int x = 0; x < 5; x++)
    {
        /* Indicamos que cada juego se encuentra actualmente libre */
        vaciar_tablero(&games[x]);
        games[x].libre = 1;
        /* Dejamos los sockets en -1 por convención */
        games[x].jugadores[0] = -1;
        games[x].jugadores[1] = -1;

        /* La partida aún no tiene un identificador */
        games[x].game_id = -1;
        /* Cada partida tiene un contador de jugadas para verificar empates */
        games[x].jugadas_realizadas = 0;
    }

    /* Ahora empezamos a escuchar para que lleguen los usuarios*/

    listen(socket_server, 10);

    int cliente_esperando = -1;

    /*
    Creamos una conexión en -1, de esta manera, cuando el accept()
    reciba una conexión real, actualizará el valor y se podrá
    trabajar de forma lógica
    */

    for (;;)
    {

        int cliente_nuevo = accept(socket_server, NULL, NULL);
        /* Esperamos una conexión en el socket del server, pero no nos interesa quien se conecta */
        int slot_libre = buscar_slot(games);
        if (slot_libre != -1)
        {
            if (cliente_esperando == -1)
            { /* Si no hay cliente esperando, este empieza a esperar.*/
                cliente_esperando = cliente_nuevo;
                send(cliente_esperando, &ESPERANDO_RIVAL, sizeof(ESPERANDO_RIVAL), 0);
                // El código 3 corresponderá a "Esperando contrincante"
                continue;
                /* Se usa un continue para saltarse lo demás y volver a esperar */
            }
            games[slot_libre].jugadores[0] = cliente_esperando;
            games[slot_libre].jugadores[1] = cliente_nuevo;
            games[slot_libre].game_id = siguiente_game_id++;
            cliente_esperando = -1;
            games[slot_libre].libre = 0;
            nTask partida = nEmitTask((int (*)())jugar_partida, &games[slot_libre]);
        }
        else
        {
            PutObj(en_espera, &cliente_nuevo);
        }
        /* Utilizando nSystem llamamos a un subproceso para facilitar esta parte */
    }
    close(cliente_esperando);
    shutdown_server(socket_server);
    /* Aquí matamos todos los sockets */
}