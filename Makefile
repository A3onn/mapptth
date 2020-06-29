CC=gcc
CCFLAGS=-Wall -lpthread -lcurl -I lexbor/ -L lexbor/ -llexbor -Wl,-Rlexbor/

main: main.o linked_list_urls.o
	$(CC) $(CCFLAGS) -o main main.o linked_list_urls.o

main.o: main.c

linked_list_urls.o: linked_list_urls.c linked_list_urls.h

.PHONY: clean

clean:
	rm *.o main
