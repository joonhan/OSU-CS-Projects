CC = gcc
CFLAG = -g

chatclient:	chatclient.o
	$(CC) $(CFLAGS) chatclient.o -o chatclient

chatclient.o:	chatclient.c
	$(CC) $(CFLAGS) -c chatclient.c

clean:	
		rm -f *.o 

cleanall:
		rm -f *.o
		rm -f chatclient
