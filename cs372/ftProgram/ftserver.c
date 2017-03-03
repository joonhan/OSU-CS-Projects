/* CS372 Winter2017
 * Joon Han
 * Program 2, ftserver.c
 * This runs the server side of a simple file transfer system.
 */

#include <stdio.h>
#include <stdlib.h>


int startConnection() {
	
	//bind()
	//listen()
	return 0; 
}
//
//
//sendFile()



int main(int argc, char* argv[]) {

	//validate commandline parameters
	if (argc != 2) {
		fprintf(stderr, "Usage: %s [port number]\n", argv[0]);
		exit(1);
	}	
	
	int socketFD;	
	socketFD = startConnection(char* argv[1]);




	return 0;
}
