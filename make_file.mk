all:
	gcc -std=c99 -Wall -o server.o server.c
	gcc -std=c99 -Wall -o client.o client.c
