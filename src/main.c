#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "server.h"

/*

#T1
#4 de Octubre 2026
#Claudio Rodríguez Parra
#Jesús Vivanco Zambrano
#Luciano Venegas Castro

*/

void nMain(int argc, char const *argv[])
{

    int codigo = 0;

    for (;;)
    {
        if (codigo == 1)
        {
        }
        else if (codigo == 2)
        {
            server();
        }
        else
        {

            printf("Escribe el código asociado al rol \n1 -> Cliente \n2 -> Servidor \nEscribe aquí:");
            scanf("%d", &codigo);
        }
    }
}