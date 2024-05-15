CC=gcc
CFLAGS=-I.

client: client.c helper.c buffer.c
	$(CC) -o client client.c helper.c buffer.c -Wall 

run: client
	./client

clean:
	rm -f *.o client
