CC=gcc 
CFLAGS= -Wall -g
all: file_server file_client

server: file_server.c
	$(CC) $(CFLAGS) file_server file_server.c
	
client: file_client.c
	$(CC) $(CFLAGS) file_client file_client.c

clean:
	rm -f file_server file_client
