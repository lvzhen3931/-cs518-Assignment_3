CC = gcc
DEBUG = -g
CFLAGS = -c -Wall $(DEBUG)
LFLAGS = -Wall $(DEBUG)

all: client server

client: libnetfiles.o
	$(CC) $(LFLAGS) libnetfiles.o -o client

libnetfiles.o: libnetfiles.c libnetfiles.h
	$(CC) $(CFLAGS) libnetfiles.c 

server: netfileserver.o
	$(CC) $(LFLAGS) netfileserver.o -lpthread -o server

netfileserver.o: netfileserver.c libnetfiles.h
	$(CC) $(CFLAGS) netfileserver.c

clean:
	\rm *.o client server


