// CS372 Winter2017
// Joon Han
// Program 1, chatclient.c
// This program runs the 'client' side of a simple chat program. 
// ** Parts of this code is referenced from a program I wrote in CS344,
// the content of which heavily references Beej's Guide to Network Programming

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>

#define MAX_NAME_SIZE 64
#define MAX_MSG_SIZE 500


//establish socket connection
int startConnection(char* argv1, char* argv2) {

	int sockFD, portNumber; 

	//set up socket address struct
	struct sockaddr_in serverAddress; 

	//store a pointer to a host entry in the database
	struct hostent* serverHostInfo; 
	
	//zero out serveraddress
	memset( (char*)&serverAddress, '\0', sizeof(serverAddress)); 
	
	//convert port# string to integer
	portNumber = atoi(argv2); 
	
	//designate the address family that socket can communicate with and port #
	serverAddress.sin_family = AF_INET; 
	serverAddress.sin_port = htons(portNumber); 	

	//check if hostName is valid and exists in database; if not return error
	serverHostInfo = gethostbyname(argv1); 
	if(serverHostInfo == NULL) {
		fprintf(stderr, "Client error: no such host\n"); 
		exit(1); 
	}

	//copy host address into socket struct
	memcpy( (char*)&serverAddress.sin_addr.s_addr, (char*)serverHostInfo->h_addr, serverHostInfo->h_length);

	//attempt to create socket; display error if it fails
	if((sockFD = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		fprintf(stderr, "Client error: failed to create socket\n");
		exit(1); 
	}

	//attempt to connect socket to server; display error if it fails
	if(connect(sockFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
		fprintf(stderr, "Client error: failed to connect socket\n");
		exit(1);  
	}

	return sockFD; 
}

//read user input, validate it, then save as user name
void storeUserName(char* userName) {
	unsigned int nameLength; 

	//zero out char array	
	memset(userName, '\0', MAX_NAME_SIZE);
 
	//store length of name
	nameLength = strlen(fgets(userName, MAX_NAME_SIZE, stdin)); 
	
	//input size validation (fgets() appends '\n' to end) 
	while (nameLength > 11 || nameLength < 2) {
		printf("Invalid user name. Try again: ");
		memset(userName, '\0', MAX_NAME_SIZE); 
		nameLength = strlen(fgets(userName, MAX_NAME_SIZE, stdin)); 
	} 

	//remove trailing '\n'
	if(nameLength > 0 && userName[nameLength - 1] == '\n') {
		userName[--nameLength] = '\0'; 
	}

}	

//read user input and save as user's message
void storeUserMsg(char* userMsg) {
	unsigned int msgLength; 

	//zero out message
	memset(userMsg, '\0', MAX_MSG_SIZE); 
	
	//read user message
	fgets(userMsg, MAX_MSG_SIZE, stdin);

	//store msg length
	msgLength = strlen(userMsg); 

	//remove trailing '\n'
	if(msgLength > 0 && userMsg[msgLength - 1] == '\n') {
		userMsg[--msgLength] = '\0';
	}	

}

//send message through socket
void sendMsg(int sockFD, const void* msg, int size, int flag) {
	
	int numOfBytes; 
	
	if ((numOfBytes = send(sockFD, msg, size, flag) < 0)) {
		fprintf(stderr, "Client error: failed to send message to server\n"); 
		exit(1); 
	}

}

//receive message through socket
void recvMsg(int sockFD, char* msg, int size, int flag) {
	
	int numOfBytes; 
	
	if ((numOfBytes = recv(sockFD, msg, size, flag) < 0)) {
		fprintf(stderr, "Client error: failed to recv server name from server\n"); 
		exit(1); 
	}
}


int main(int argc, char* argv[]) {

	//check for 3 CMD line arguments, return usage statement if not given
	if(argc != 3) {
		fprintf(stderr, "usage: %s hostname port\n", argv[0]);
		exit(1); 
	}

	int socketFD; 
	char userName[MAX_NAME_SIZE];
	char userMsg[MAX_MSG_SIZE]; 
	char serverName[MAX_NAME_SIZE]; 
	char serverMsg[MAX_MSG_SIZE]; 
	memset(serverMsg, '\0', MAX_MSG_SIZE); 
	
	//establish socket connection and return socket file descriptor
	socketFD = startConnection(argv[1], argv[2]); 

	//prompt and get handle from user
	printf("Please enter your user name (up to 10 char): "); 
	storeUserName(userName); 
 
	//loop until '\quit' is typed
	while(1) {

		//print user handle and prompt for message
		printf("%s> ", userName); 
		storeUserMsg(userMsg); 

		//if user types "\quit" close socket and exit
		if(strncmp(userMsg, "\\quit", 5) == 0) {
			printf("Exiting program...\n"); 
			//shutdown(socketFD, 2); 
			close(socketFD); 
			exit(0); 
		}
		
		//otherwise send user name and message to server
		sendMsg(socketFD, userName, sizeof(userName), 0);
		sendMsg(socketFD, userMsg, sizeof(userMsg), 0);

		//receive server name from server
		recvMsg(socketFD, serverName, 6, 0);
		
		//if server sends no message, it indicates connection is closed 
		if (strcmp(serverName, "") == 0) {
			printf("Connection to server lost. Exiting...\n");
			close(socketFD);  
			exit(1); 
		}
	
		//print server name
		printf("%s> ", serverName);
 	
		//receive server's message and print it
		recvMsg(socketFD, serverMsg, sizeof(serverMsg), 0); 
		printf("%s\n", serverMsg); 

		//zero out server name and message to remove chars from previous iteration
		memset(serverName, '\0', MAX_NAME_SIZE); 
		memset(serverMsg, '\0', MAX_MSG_SIZE); 
	}

	return 0; 
}
