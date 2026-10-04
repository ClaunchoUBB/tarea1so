#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <strings.h>
#include <nSystem.h>
#include <fifoqueues.h>
#include "server.h"
#include <poll.h>

/*

#T1
#4 de Octubre 2026
#Claudio Rodríguez Parra

*/

#define NUM_PARTIDAS 5

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
int TERMINADO = 183;
int ESPERA = 6;

const int combinaciones[8][3] = {
    {0, 1, 2},
    {3, 4, 5},
    {6, 7, 8},
    {0, 3, 6},
    {1, 4, 7},
    {2, 5, 8},
    {0, 4, 8},
    {2, 4, 6}};

struct Game
{
    /*
    Esta estructura, respresenta en el sentido más literal posible
    el concepto de un espacio de juego, posee su tablero, el número
    de jugadas realizadas (para declarar empate) y los jugadores
    además de un identificador único.
    */
    int game_id;
    int libre;
    char tablero[9];
    /*
    [0][1][2]
    [3][4][5]  == [0][1][2][3][4][5][6][7][8]
    [6][7][8]
    */
    int jugadas_realizadas;
    int jugadores[2];
};

struct Game *games = NULL; // Estructura global para los juegos

static int enviar_todo(int fd, const void *buf, size_t n)
{
    /*
    Esta función abstrae el uso del send()
    y espera errores.
    */

    const char *p = buf;
    size_t enviado = 0;

    while (enviado < n)
    {
        ssize_t r = send(fd, p + enviado, n - enviado, 0);
        if (r < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }
            if (errno != EPIPE && errno != ECONNRESET)
            {
                perror("send");
            }
            return -1;
        }
        if (r == 0)
        {
            return -1;
        }
        enviado += (size_t)r;
    }
    return 0;
}

static int recibir_todo(int fd, void *buf, size_t n)
{

    /*
    Esta func abstrae el uso de recieve. Retorna 0 si todo salió bien y -1 si el
    cliente cerró la conexión (recv == 0) o hubo un error.
    */
    char *p = buf;
    size_t recibido = 0;

    while (recibido < n)
    {
        ssize_t r = recv(fd, p + recibido, n - recibido, 0);
        if (r < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }
            if (errno != ECONNRESET)
            {
                perror("recv");
            }
            return -1;
        }
        if (r == 0)
        {
            /* El cliente cerró la conexión antes de tiempo */
            return -1;
        }
        recibido += (size_t)r;
    }
    return 0;
}

static void cerrar_fd(int *fd)
{

    /* Lit lo que dice el nombre */
    if (*fd != -1)
    {
        if (close(*fd) < 0)
        {
            perror("close");
        }
        *fd = -1;
    }
}

void vaciar_tablero(struct Game *game)
{
    /*
    Esta función existe para que a la hora de crear un juego
    el tablero siempre esté lleno de ' '
    */
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
    if (games != NULL)
    {
        for (int i = 0; i < NUM_PARTIDAS; i++)
        {
            if (games[i].libre == 0)
            {
                cerrar_fd(&games[i].jugadores[0]);
                cerrar_fd(&games[i].jugadores[1]);
            }
        }

        free(games);
        games = NULL;
    }

    /* Cerramos el socket principal del servidor. */
    if (socket_server >= 0)
    {
        close(socket_server);
    }
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

    if (partida->jugadas_realizadas < 9)
    {
        // Jugada válida, nadie gana aún
        return 0;
    }
    else
    {
        // Se llegó al límite sin ganador: empate
        return 4;
    }
}

/*
Termina la partida porque el jugador "caido" se desconectó o falló su
socket. Se cierra su conexión y, si el rival sigue conectado, se le
avisa que ganó por abandono. Retorna -2.
*/
static int abandono(struct Game *partida, int caido)
{
    fprintf(stderr, "[partida %d] jugador %d desconectado, gana el rival\n",
            partida->game_id, caido);

    cerrar_fd(&partida->jugadores[caido]);
    /* Si el rival también está caído, este send falla y no pasa nada */
    enviar_todo(partida->jugadores[1 - caido], &VICTORIA, sizeof(VICTORIA));
    cerrar_fd(&partida->jugadores[1 - caido]);
    return -2;
}

/* Cierra ambos sockets de una partida que terminó normalmente */
static void cerrar_partida(struct Game *partida)
{
    cerrar_fd(&partida->jugadores[0]);
    cerrar_fd(&partida->jugadores[1]);
}

/*
Retorna:
   0  partida terminada normalmente (victoria o empate)
   1  terminó por exceso de jugadas inválidas
  -1  error interno
  -2  terminó por desconexión de un jugador
*/
int jugar_partida(struct Game *partida)
{
    /*
    Esta sí que es una de las más importantes

    La idea principal es que reciba una partida y se comunique
    constantemente con ambos jugadores, asímismo, constamente
    cambia entre el jugador 1 y el 2 en los ints a y b
    esta función no es la encargada de chekear las jugadas
    simplemente comunica los dos jugadores con el tablero
    y el servidor
    */

    const char signos[2] = {'X', 'O'};
    int turno = 0;
    int jugada_buffer;
    int status_jugada;
    int contador_errores;

    vaciar_tablero(partida);
    partida->jugadas_realizadas = 0;

    if (enviar_todo(partida->jugadores[0],
                    &signos[0],
                    sizeof(signos[0])) < 0)
    {
        return abandono(partida, 0);
    }

    if (enviar_todo(partida->jugadores[1],
                    &signos[1],
                    sizeof(signos[1])) < 0)
    {
        return abandono(partida, 1);
    }

    for (;;)
    {
        int a = partida->jugadores[turno];     // juega
        int b = partida->jugadores[1 - turno]; // espera
        contador_errores = 0;

        if (recibir_todo(a, &jugada_buffer, sizeof(jugada_buffer)) < 0)
        {
            return abandono(partida, turno);
        }
        status_jugada = check_jugada(jugada_buffer, signos[turno], partida);

        while (status_jugada == -1)
        {
            if (contador_errores == 3)
            {
                /* Envíos "mejor esfuerzo": la partida termina igual */
                enviar_todo(a, &DEMASIADOS_ERRORES, sizeof(DEMASIADOS_ERRORES));
                enviar_todo(b, partida->tablero, sizeof(partida->tablero));
                enviar_todo(b, &VICTORIA, sizeof(VICTORIA));
                cerrar_partida(partida);
                nExitTask(1);
                return 1;
            }
            if (enviar_todo(a, &JUGADA_INVALIDA, sizeof(JUGADA_INVALIDA)) < 0)
            {
                return abandono(partida, turno);
            }
            contador_errores++;
            if (recibir_todo(a, &jugada_buffer, sizeof(jugada_buffer)) < 0)
            {
                return abandono(partida, turno);
            }
            status_jugada = check_jugada(jugada_buffer, signos[turno], partida);
        }

        switch (status_jugada)
        {
        case 1: // Victoria de a
            enviar_todo(a, partida->tablero, sizeof(partida->tablero));
            enviar_todo(b, partida->tablero, sizeof(partida->tablero));
            enviar_todo(a, &VICTORIA, sizeof(VICTORIA));
            enviar_todo(b, &DERROTA, sizeof(DERROTA));
            cerrar_partida(partida);
            return 0;

        case 4: // Empate
            enviar_todo(a, partida->tablero, sizeof(partida->tablero));
            enviar_todo(b, partida->tablero, sizeof(partida->tablero));
            enviar_todo(a, &EMPATE, sizeof(EMPATE));
            enviar_todo(b, &EMPATE, sizeof(EMPATE));
            cerrar_partida(partida);
            nExitTask(0);
            return 0;

        case 0: // Jugada válida, la partida sigue
            if (enviar_todo(a, &JUGADA_ACEPTADA, sizeof(JUGADA_ACEPTADA)) < 0 ||
                enviar_todo(a, partida->tablero, sizeof(partida->tablero)) < 0)
            {
                return abandono(partida, turno);
            }
            if (enviar_todo(b, partida->tablero, sizeof(partida->tablero)) < 0 ||
                enviar_todo(b, &TU_TURNO, sizeof(TU_TURNO)) < 0)
            {
                return abandono(partida, 1 - turno);
            }
            if (enviar_todo(a, &ESPERA_TURNO, sizeof(ESPERA_TURNO)) < 0)
            {
                return abandono(partida, turno);
            }
            break;

        default:
            enviar_todo(a, &ERROR_INTERNO, sizeof(ERROR_INTERNO));
            enviar_todo(b, &ERROR_INTERNO, sizeof(ERROR_INTERNO));
            cerrar_partida(partida);
            fprintf(stderr, "Resultado de jugada no manejado: %d\n", status_jugada);
            nExitTask(-1);
            return -1; // Nunca llegua hasta aquí
        }
        turno = 1 - turno; // cambia el turno
    }
}

/*
Tarea que ejecuta una partida completa y, al terminar, deja libre el slot.
Es lo último que hace, después de haber cerrado los sockets.
*/
int tarea_partida(struct Game *partida)
{
    int resultado = jugar_partida(partida);
    partida->game_id = -1;
    partida->libre = 1;
    return resultado;
}

int buscar_slot(struct Game *partidas)
{
    /*
    Busca un slot disponible entre las partidas alojadas en mem
    */
    for (int i = 0; i < NUM_PARTIDAS; i++)
    {
        if (partidas[i].libre == 1)
        {
            return i;
        }
    }
    return -1;
}

/* Saca un cliente (fd) de la cola de espera y libera su nodo */
static int sacar_cliente(FifoQueue cola)
{
    int *p = GetObj(cola);
    int fd = *p;
    free(p);
    return fd;
}

/* Asigna dos clientes a un slot y lanza la tarea de la partida */
static void iniciar_partida(int slot, int fd0, int fd1, int *siguiente_game_id)
{
    games[slot].jugadores[0] = fd0;
    games[slot].jugadores[1] = fd1;
    games[slot].game_id = (*siguiente_game_id)++;
    games[slot].libre = 0;

    nTask t = nEmitTask((int (*)())tarea_partida, &games[slot]);

    if (t == NULL)
    {
        fprintf(stderr, "No se pudo crear la tarea de la partida %d\n", games[slot].game_id);
        cerrar_fd(&games[slot].jugadores[0]);
        cerrar_fd(&games[slot].jugadores[1]);
        games[slot].game_id = -1;
        games[slot].libre = 1;
    }
}

#include <poll.h>

/*Hace poll en las conexiones para evitar un deadlock en accept()*/
static int hay_conexion(int socket_server)
{
    struct pollfd pfd;
    int r;

    pfd.fd = socket_server;
    pfd.events = POLLIN;
    pfd.revents = 0;

    r = poll(&pfd, 1, 0); /* timeout 0: no bloquea */
    if (r < 0)
    {
        if (errno == EINTR)
            return 0;
        perror("poll");
        return -1;
    }
    return (r > 0 && (pfd.revents & POLLIN)) ? 1 : 0;
}

void server()
{
    struct sockaddr_in direccion_propia;
    socklen_t size_direccion_propia = sizeof(direccion_propia);
    FifoQueue en_espera = MakeFifoQueue();
    int siguiente_game_id = 1;
    int socket_server = -1;
    /*
    Si un cliente se desconecta y le hacemos send(), el SO manda SIGPIPE
    y mataría todo el servidor. Lo ignoramos: send() retornará -1 con EPIPE.
    */
    signal(SIGPIPE, SIG_IGN);

    direccion_propia.sin_family = AF_INET;
    direccion_propia.sin_port = 0;
    direccion_propia.sin_addr.s_addr = htonl(INADDR_ANY);
    bzero(&(direccion_propia.sin_zero), 8);

    /* Creamos el fichero descriptor del socket */
    socket_server = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_server < 0)
    {
        perror("socket");
        goto fin;
    }

    /* Enlazamos el socket con la dirección de entrada */
    if (bind(socket_server, (struct sockaddr *)(&direccion_propia), size_direccion_propia) < 0)
    {
        perror("bind");
        goto fin;
    }

    /* Le pedimos el nombre para que el usuario conozca el puerto */
    if (getsockname(socket_server, (struct sockaddr *)&direccion_propia, &size_direccion_propia) < 0)
    {
        perror("getsockname");
        goto fin;
    }

    printf("Esperando en todas las interfaces de red\nPuerto:%d \n", ntohs(direccion_propia.sin_port));

    /* Alojamos las partidas en la memoria */
    games = malloc(NUM_PARTIDAS * sizeof(*games));
    if (games == NULL)
    {
        perror("malloc");
        goto fin;
    }

    for (int x = 0; x < NUM_PARTIDAS; x++)
    {
        vaciar_tablero(&games[x]);
        games[x].libre = 1;
        games[x].jugadores[0] = -1;
        games[x].jugadores[1] = -1;
        games[x].game_id = -1;
        games[x].jugadas_realizadas = 0;
    }

    /* Ahora empezamos a escuchar para que lleguen los usuarios */
    if (listen(socket_server, 10) < 0)
    {
        perror("listen");
        goto fin;
    }

    int cliente1 = -1;
    int cliente2 = -1;

    for (;;) // Demonizamos
    {
        /* Esperamos una conexión en el socket del server, pero no nos interesa quien se conecta */
        int slot_libre = buscar_slot(games);
        if (slot_libre != -1) // Si hay un slot libre
        {

            if (LengthFifoQueue(en_espera) >= 2) // Y una pareja esperando, jugamos
            {
                cliente1 = sacar_cliente(en_espera);
                if (cliente1 == -1)
                {
                    perror("GetObj");
                    break;
                }

                cliente2 = sacar_cliente(en_espera);
                if (cliente2 == -1)
                {
                    perror("GetObj");
                    close(cliente1);
                    break;
                }
            }
            else if (LengthFifoQueue(en_espera) == 1) // Si hay uno solo, esperamos
            {
                cliente1 = sacar_cliente(en_espera);
                cliente2 = accept(socket_server, NULL, NULL);
                if (cliente2 == -1)
                {
                    perror("accept");
                    break;
                }
            }
            else
            {
                cliente1 = accept(socket_server, NULL, NULL);
                if (cliente1 == -1)
                {
                    perror("accept");
                    break;
                }

                if (cliente2 == -1)
                { /* Si no hay cliente esperando, este empieza a esperar.*/
                    cliente2 = cliente1;
                    if (send(cliente2, &ESPERANDO_RIVAL, sizeof(ESPERANDO_RIVAL), 0) == -1)
                    {
                        perror("send");
                        close(cliente2);
                        break;
                    }
                    continue;
                    /* Se usa un continue para saltarse lo demás y volver a esperar */
                }
            }

            iniciar_partida(slot_libre, cliente2, cliente1, &siguiente_game_id);
            cliente1=-1;
            cliente2=-1;
        }
        else
        {
            int *cliente_nuevo;
            cliente_nuevo = malloc(sizeof(int));
            *cliente_nuevo = accept(socket_server, NULL, NULL);
            if (*cliente_nuevo == -1)
            {
                perror("accept");
                free(cliente_nuevo);
                break;
            }
            send(*cliente_nuevo, &ESPERA, sizeof(ESPERA), 0);
            PutObj(en_espera, cliente_nuevo);
        }
        /* Utilizando nSystem llamamos a un subproceso para facilitar esta parte */
    }
fin:
    /* Cerramos los clientes que seguían en la cola de espera */
    while (LengthFifoQueue(en_espera) > 0)
    {
        int fd = sacar_cliente(en_espera);
        close(fd);
    }
    DestroyFifoQueue(en_espera);
    shutdown_server(socket_server);
}