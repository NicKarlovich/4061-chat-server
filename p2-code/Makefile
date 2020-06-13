include test_Makefile

all : bl_server bl_client simpio_demo

bl_server :
		gcc -Wall -g -c bl_server.c
		gcc -Wall -g -c server_funcs.c
		gcc -Wall -g -c util.c
		gcc -Wall -g -o bl_server bl_server.o server_funcs.o util.o -lpthread

bl_client : bl_server
		gcc -Wall -g -c bl_client.c
		gcc -Wall -g -c simpio.c
		gcc -Wall -g -o bl_client bl_client.o simpio.o util.o -lpthread

clean:
	rm bl_server bl_client *.o
