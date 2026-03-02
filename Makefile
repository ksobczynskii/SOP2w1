CC=gcc
CFLAGS= -std=gnu99 -Wall

server: server.c
	${CC} ${CFLAGS} server.c -o server

client: client.c
	${CC} ${CFLAGS} client.c -o client


.PHONY: clean

clean:
	rm -rf client server
