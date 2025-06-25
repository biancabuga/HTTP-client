CC=gcc
CFLAGS=-I. -Wall
LIBS=-lm

client: client.c requests.c helpers.c buffer.c parson.c
	$(CC) -o client client.c requests.c helpers.c buffer.c parson.c $(CFLAGS) $(LIBS)

run: client
	./client

clean:
	rm -f *.o client
