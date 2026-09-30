# Para usar este Makefile es necesario definir la variable
# de ambiente NSYSTEM con el directorio en donde se encuentra
# la raiz de nSystem.  En csh esto se hace con:
#
#   setenv NSYSTEM ~cc41b/nSystem95
#
# Para compilar ingrese make APP=<ejemplo>
#
# Ej: make APP=fibonacci
#


NSYSTEM=./libs/nsystem64-beta3

CFLAGS=-ggdb -I$(NSYSTEM)/include -I$(NSYSTEM)/src
LDFLAGS=-ggdb -L$(NSYSTEM)/lib
LDLIBS=-lnSys

TARGET=tarea1so

SRC=src/main.c src/server.c src/client.c
OBJ=$(SRC:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJ)
	gcc $(LDFLAGS) $(OBJ) -o $@ $(LDLIBS)

src/%.o: src/%.c
	gcc -c $(CFLAGS) $< -o $@

clean:
	rm -f src/*.o $(TARGET)