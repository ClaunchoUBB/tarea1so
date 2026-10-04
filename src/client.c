#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

/*

#T1
#4 de Octubre 2026
#Claudio Rodríguez Parra

*/

/*
 * Codigos enviados por el servidor.
 */
#define ESPERANDO_RIVAL 3
#define TU_TURNO 1
#define ESPERA_TURNO 2
#define JUGADA_ACEPTADA 0
#define JUGADA_INVALIDA -1
#define DERROTA 5
#define VICTORIA 10
#define DEMASIADOS_ERRORES -3
#define ERROR_INTERNO 200
#define EMPATE 4
#define TERMINADO 183
#define ESPERA 6

/*
 * Recibe exactamente n bytes.
 *
 * Retorna:
 *  0 si se recibieron todos los bytes.
 * -1 si ocurrio un error o el servidor cerro
 *    la conexion.
 */
static int recibir_todo(int fd, void *buf, size_t n)
{
    char *p;
    size_t recibido;
    ssize_t r;

    p = (char *)buf;
    recibido = 0;

    while (recibido < n)
    {
        r = recv(fd, p + recibido, n - recibido, 0);

        if (r < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }

            perror("recv");
            return -1;
        }

        if (r == 0)
        {
            return -1;
        }

        recibido += (size_t)r;
    }

    return 0;
}

/*
 * Envia exactamente n bytes.
 *
 * Retorna:
 *  0 si se enviaron todos los bytes.
 * -1 si ocurrio un error.
 */
static int enviar_todo(int fd, const void *buf, size_t n)
{
    const char *p;
    size_t enviado;
    ssize_t r;

    p = (const char *)buf;
    enviado = 0;

    while (enviado < n)
    {
        r = send(fd, p + enviado, n - enviado, 0);

        if (r < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }

            perror("send");
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

/*
 * Muestra el tablero recibido.
 *
 * El servidor utiliza:
 *
 * [0][1][2]
 * [3][4][5]
 * [6][7][8]
 */
static void mostrar_tablero(char tablero[9])
{
    printf("\n");
    printf(" %c | %c | %c\n",
           tablero[0], tablero[1], tablero[2]);

    printf("---+---+---\n");

    printf(" %c | %c | %c\n",
           tablero[3], tablero[4], tablero[5]);

    printf("---+---+---\n");

    printf(" %c | %c | %c\n",
           tablero[6], tablero[7], tablero[8]);

    printf("\n");
}

/*
 * Solicita una fila y una columna al usuario.
 *
 * Las filas y columnas que ingresa el usuario van
 * desde 1 hasta 3.
 *
 * Retorna la posicion equivalente del tablero:
 *
 * 1 1 -> 0
 * 1 2 -> 1
 * 1 3 -> 2
 * 2 1 -> 3
 * ...
 * 3 3 -> 8
 */
static int solicitar_jugada(void)
{
    int fila;
    int columna;

    for (;;)
    {
        printf("Ingresa la fila (1-3): ");

        if (scanf("%d", &fila) != 1)
        {
            printf("Entrada invalida.\n");

            while (getchar() != '\n')
            {
                /* Vaciar entrada */
            }

            continue;
        }

        printf("Ingresa la columna (1-3): ");

        if (scanf("%d", &columna) != 1)
        {
            printf("Entrada invalida.\n");

            while (getchar() != '\n')
            {
                /* Vaciar entrada */
            }

            continue;
        }

        if (fila < 1 || fila > 3 ||
            columna < 1 || columna > 3)
        {
            printf("La fila y columna deben estar entre 1 y 3.\n");
            continue;
        }

        return (fila - 1) * 3 + (columna - 1);
    }
}

int cliente(void)
{
    char ip[64];
    int puerto;
    int socket_cliente;
    int codigo;
    int jugada;
    int simbolo;
    char tablero[9];

    struct sockaddr_in servidor;

    /*
     * Solicitar IP.
     */
    printf("Ingresa la IP: ");

    if (scanf("%63s", ip) != 1)
    {
        fprintf(stderr, "No se pudo leer la IP.\n");
        return EXIT_FAILURE;
    }

    /*
     * Solicitar puerto.
     */
    printf("Ingresa el puerto: ");

    if (scanf("%d", &puerto) != 1)
    {
        fprintf(stderr, "Puerto invalido.\n");
        return EXIT_FAILURE;
    }

    if (puerto < 1 || puerto > 65535)
    {
        fprintf(stderr,
                "El puerto debe estar entre 1 y 65535.\n");

        return EXIT_FAILURE;
    }

    /*
     * Crear socket TCP.
     */
    socket_cliente = socket(AF_INET, SOCK_STREAM, 0);

    if (socket_cliente < 0)
    {
        perror("socket");
        return EXIT_FAILURE;
    }

    /*
     * Configurar direccion del servidor.
     */
    memset(&servidor, 0, sizeof(servidor));

    servidor.sin_family = AF_INET;
    servidor.sin_port = htons((unsigned short)puerto);

    if (inet_pton(AF_INET, ip, &servidor.sin_addr) <= 0)
    {
        fprintf(stderr, "La IP ingresada no es valida.\n");

        close(socket_cliente);
        return EXIT_FAILURE;
    }

    /*
     * Conectarse al servidor.
     */
    if (connect(socket_cliente,
                (struct sockaddr *)&servidor,
                sizeof(servidor)) < 0)
    {
        perror("connect");

        close(socket_cliente);
        return EXIT_FAILURE;
    }

    printf("Conectado al servidor.\n");

    /*
     * Esperamos el primer mensaje del servidor.
     *
     * Puede ser:
     *
     * ESPERANDO_RIVAL
     *
     * o directamente el simbolo asignado.
     */
    if (recibir_todo(socket_cliente,
                     &codigo,
                     sizeof(codigo)) < 0)
    {
        fprintf(stderr,
                "El servidor cerro la conexion.\n");

        close(socket_cliente);
        return EXIT_FAILURE;
    }

    /*
     * Si somos el primer cliente, esperamos
     * a que llegue nuestro rival.
     */
    if (codigo == ESPERANDO_RIVAL)
    {
        printf("Esperando rival...\n");

        if (recibir_todo(socket_cliente,
                         &simbolo,
                         sizeof(simbolo)) < 0)
        {
            fprintf(stderr,
                    "El servidor cerro la conexion.\n");

            close(socket_cliente);
            return EXIT_FAILURE;
        }
    }
    else
    {
        /*
         * Si no recibimos ESPERANDO_RIVAL,
         * el codigo recibido debe ser nuestro
         * simbolo.
         */
        simbolo = codigo;
    }

    /*
     * Mostrar simbolo asignado.
     */
    if (simbolo == 'X' || simbolo == 'O')
    {
        printf("Tu simbolo es: %c\n", (char)simbolo);
    }
    else
    {
        fprintf(stderr,
                "El servidor envio un simbolo invalido: %d\n",
                simbolo);

        close(socket_cliente);
        return EXIT_FAILURE;
    }

    /*
     * Bucle principal de la partida.
     */
    for (;;)
    {
        /*
         * Todos los mensajes de control del servidor
         * son int.
         */
        if (recibir_todo(socket_cliente,
                         &codigo,
                         sizeof(codigo)) < 0)
        {
            fprintf(stderr,
                    "El servidor cerro la conexion.\n");

            break;
        }

        switch (codigo)
        {
        /*
         * Es nuestro turno.
         */
        case TU_TURNO:

            printf("\nEs tu turno.\n");

            /*
             * Obtener la jugada del usuario.
             */
            jugada = solicitar_jugada();

            /*
             * Enviar posicion 0-8 al servidor.
             */
            if (enviar_todo(socket_cliente,
                            &jugada,
                            sizeof(jugada)) < 0)
            {
                fprintf(stderr,
                        "No se pudo enviar la jugada.\n");

                close(socket_cliente);
                return EXIT_FAILURE;
            }

            break;

        /*
         * El rival esta jugando.
         */
        case ESPERA_TURNO:

            printf("Esperando el turno del rival...\n");

            break;

        /*
         * Nuestra jugada fue aceptada.
         *
         * El servidor envia inmediatamente
         * el tablero.
         */
        case JUGADA_ACEPTADA:

            if (recibir_todo(socket_cliente,
                             tablero,
                             sizeof(tablero)) < 0)
            {
                fprintf(stderr,
                        "No se pudo recibir el tablero.\n");

                close(socket_cliente);
                return EXIT_FAILURE;
            }

            mostrar_tablero(tablero);

            break;

        /*
         * La jugada fue invalida.
         *
         * No solicitamos la jugada aqui inmediatamente:
         * el flujo vuelve a esperar TU_TURNO.
         *
         * Sin embargo, tu servidor actualmente envia
         * JUGADA_INVALIDA y luego espera otra jugada
         * por el mismo socket, por lo que debemos
         * solicitarla nuevamente aqui.
         */
        case JUGADA_INVALIDA:

            printf("Jugada invalida. Intenta nuevamente.\n");

            jugada = solicitar_jugada();

            if (enviar_todo(socket_cliente,
                            &jugada,
                            sizeof(jugada)) < 0)
            {
                fprintf(stderr,
                        "No se pudo enviar la jugada.\n");

                close(socket_cliente);
                return EXIT_FAILURE;
            }

            break;

        /*
         * Ganamos.
         *
         * El servidor envia primero el tablero
         * y despues VICTORIA.
         */
        case VICTORIA:

            if (recibir_todo(socket_cliente,
                             tablero,
                             sizeof(tablero)) < 0)
            {
                fprintf(stderr,
                        "No se pudo recibir el tablero final.\n");

                close(socket_cliente);
                return EXIT_FAILURE;
            }

            mostrar_tablero(tablero);

            printf("Ganaste la partida.\n");

            close(socket_cliente);
            return EXIT_SUCCESS;

        /*
         * Perdimos.
         *
         * El servidor envia primero el tablero
         * y despues DERROTA.
         */
        case DERROTA:

            if (recibir_todo(socket_cliente,
                             tablero,
                             sizeof(tablero)) < 0)
            {
                fprintf(stderr,
                        "No se pudo recibir el tablero final.\n");

                close(socket_cliente);
                return EXIT_FAILURE;
            }

            mostrar_tablero(tablero);

            printf("Perdiste la partida.\n");

            close(socket_cliente);
            return EXIT_SUCCESS;

        /*
         * Empate.
         *
         * El servidor envia primero el tablero
         * y despues EMPATE.
         */
        case EMPATE:

            if (recibir_todo(socket_cliente,
                             tablero,
                             sizeof(tablero)) < 0)
            {
                fprintf(stderr,
                        "No se pudo recibir el tablero final.\n");

                close(socket_cliente);
                return EXIT_FAILURE;
            }

            mostrar_tablero(tablero);

            printf("La partida termino en empate.\n");

            close(socket_cliente);
            return EXIT_SUCCESS;

        /*
         * El jugador cometio demasiados errores.
         */
        case DEMASIADOS_ERRORES:

            printf("La partida termino por demasiadas "
                   "jugadas invalidas.\n");

            close(socket_cliente);
            return EXIT_FAILURE;

        /*
         * Error interno del servidor.
         */
        case ERROR_INTERNO:

            printf("Error interno del servidor.\n");

            close(socket_cliente);
            return EXIT_FAILURE;

        /*
         * Codigo desconocido.
         */
        default:

            fprintf(stderr,
                    "Codigo desconocido recibido: %d\n",
                    codigo);

            close(socket_cliente);
            return EXIT_FAILURE;
        }
    }

    close(socket_cliente);

    return EXIT_FAILURE;
}