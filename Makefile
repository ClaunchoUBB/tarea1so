# Para usar este Makefile es necesario definir la variable
# de ambiente NSYSTEM con el directorio en donde se encuentra
# la raiz de nSystem. A MENOS que se emplee simplemente la librería
# que viene con el código fuente

#T1
#4 de Octubre 2026
#Claudio Rodríguez Parra
#Jesús Vivanco Zambrano
#Luciano Venegas Castro

CFLAGS=-ggdb -I$(NSYSTEM)/include -I$(NSYSTEM)/src
LDFLAGS=-ggdb -L$(NSYSTEM)/lib
LDLIBS=-lnSys

TARGET=T1_crp

SRC=src/main.c src/server.c src/client.c

OBJ=$(SRC:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJ)
	gcc $(LDFLAGS) $(OBJ) -o $@ $(LDLIBS)

src/%.o: src/%.c
	gcc -c $(CFLAGS) $< -o $@

clean:
	rm -f src/*.o $(TARGET)