#ifndef CLIENT_H
#define CLIENT_H

/*
 * Conecta el cliente al servidor utilizando la IP y el puerto
 * indicados.
 *
 * Retorna:
 *  >= 0 : file descriptor del socket.
 *  -1   : error.
 */
int conectar_servidor(const char *ip, int puerto);

/*
 * Ejecuta la partida del cliente.
 *
 * socket_cliente: file descriptor del socket conectado.
 *
 * Retorna:
 *   0  : partida terminada correctamente.
 *  -1  : error.
 */
int jugar_cliente(int socket_cliente);

/*
 * Cierra el socket del cliente.
 */
void cerrar_cliente(int socket_cliente);

void cliente(void);

#endif