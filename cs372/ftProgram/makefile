CC = gcc
CFLAG = -g



ftserver: ftserver.o
	$(CC) $(CFLAGS) ftserver.o -o ftserver 
	
ftserver.o:	ftserver.c
	$(CC) $(CFLAGS) -c ftserver.c

#ftclient.o:	ftclient.c
#	$(CC) $(CFLAGS) -c ftclient.c

clean:	
		rm -f *.o 

cleanall:
		rm -f *.o
		rm -f ftserver
#		rm -f ftclient 
