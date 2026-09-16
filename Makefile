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


LIBNSYS= ./libs/nsystem64-beta3/lib/

CFLAGS= -ggdb -I ./libs/nsystem64-beta3/include -I ./libs/nsystem64-beta3/src
LFLAGS= -ggdb

all: $(APP)

.SUFFIXES:
.SUFFIXES: .o .c .s

.c.o .s.o:
	gcc -c $(CFLAGS) $<

$(APP): $(APP).o $(LIBNSYS)
	gcc $(LFLAGS) $@.o -o $@ $(LIBNSYS)

clean:
	rm -f *.o *~ prod-cons

cleanall:
	rm -f *.o *~ prod-cons 
