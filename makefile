CC=gcc
CFLAGS=-Wall -pthread

all: search_tool

search_tool: main.o search.o
	$(CC) $(CFLAGS) -o search_tool main.o search.o

main.o: main.c search.h
	$(CC) $(CFLAGS) -c main.c

search.o: search.c search.h
	$(CC) $(CFLAGS) -c search.c

clean:
	rm -f *.o search_tool

