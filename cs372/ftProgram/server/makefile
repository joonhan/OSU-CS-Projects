CC = gcc
CFLAGS = -g

ftserver: ftserver.o
	$(CC) $(CFLAGS) ftserver.o -o ftserver 
	
ftserver.o:	ftserver.c
	$(CC) $(CFLAGS) -c ftserver.c

clean:	
		rm -f *.o 

cleanall:
		rm -f *.o
		rm -f ftserver
