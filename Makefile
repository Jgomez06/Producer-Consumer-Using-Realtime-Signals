#C Compiler
CC = gcc
CFLAGS = -g

all: prodcon Consumer

#server object to executable
prodcon: prodcon.o 
	$(CC) -o prodcon prodcon.o

#Compile server source code 
prodcon.o: prodcon.c
	$(CC) $(CFLAGS) -c prodcon.c

#Client object to executable 
Consumer: Consumer.o 
	$(CC) -o Consumer Consumer.o

#Compile client source code
Consumer.o: Consumer.c
	$(CC) $(CFLAGS) -c Consumer.c

clean: 
	rm -f *.o
	rm -f prodcon
	rm -f Consumer
