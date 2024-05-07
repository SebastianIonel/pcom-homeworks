# Protocoale de comunicatii
# Laborator 7 - TCP
# Makefile

CFLAGS = -lm -Wall -g -Werror -Wno-error=unused-variable 

# Adresa IP a serverului
IP_SERVER = 127.0.0.1
SERVER_PORT = 12345

all: server subscriber

# Compileaza server.c
server: server.c

# Compileaza client.c
client: subscriber.c

.PHONY: build clean run_server run_subscriber

build: server subscriber

server: server.c
	gcc -o server server.c protocol.h $(CFLAGS)

subscriber: subscriber.c
	gcc -o subscriber subscriber.c protocol.h $(CFLAGS)

# Ruleaza serverul
run_server:
	./server ${PORT}

# Ruleaza clientul
run_subscriber:
	./subscriber ${ID_CLIENT} ${IP_SERVER} ${SERVER_PORT}

clean:
	rm -rf server subscriber *.o *.dSYM