CC=gcc
CFLAGS=-I. -Wall -Werror -g 

default: server

server: httpechosrv.o
	$(CC) -o server httpechosrv.o -lpthread

clean:
	rm server.o
	rm server
