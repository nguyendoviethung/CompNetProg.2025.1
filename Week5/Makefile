CC = gcc
CFLAGS = -Wall
LIBS = -lws2_32

all: server.exe client.exe

server.exe: server.c
	$(CC) $(CFLAGS) -o server.exe server.c $(LIBS)

client.exe: client.c
	$(CC) $(CFLAGS) -o client.exe client.c $(LIBS)

clean:
	del /Q server.exe client.exe 2>nul || exit 0