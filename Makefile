CC=gcc
CCFLAGS=-Wall -lpthread -lcurl -I lexbor/ -L lexbor/ -llexbor -Wl,-Rlexbor/

main: main.o
	$(CC) $(CCFLAGS) -o main main.o

main.o: main.c

.PHONY: clean

clean:
	rm *.o main
