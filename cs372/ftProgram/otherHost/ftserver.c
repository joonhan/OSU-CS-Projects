/* CS372 Winter2017
 * Joon Han
 * Program 2, ftserver.c
 * This runs the server side of a simple file transfer system.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>

int startConnection(char* argv1) {

	int sockFD, connectedSockFD, portNumber; 
	socklen_t sizeOfClientInfo; 

	struct sockaddr_in serverAddress, clientAddress; 
	struct hostent* serverHostInfo; 

	memset( (char*)&serverAddress, '\0', sizeof(serverAddress)); 
	portNumber = atoi(argv1); 
	
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_port = htons(portNumber); 	
	serverAddress.sin_addr.s_addr = INADDR_ANY; 
	
	//create a socket and report any errors	
	if ( (sockFD = socket(AF_INET, SOCK_STREAM, 0)) < 0)  {
		fprintf(stderr, "Error: server failed to create socket\n"); 
		exit(1); 
	}

	//removes the 'address in use' error when attempting to reconnect
	/*int optVal = 1;
   if (setsockopt(sockFD, SOL_SOCKETADDR, &optVal, sizeof(optVal)) < 0) {
       fprintf(stderr, "Error: attempt to reuse socket failed\n");
       exit(1);
   }*/	
	
	//bind the address
	if (bind(sockFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
		fprintf(stderr, "Error: server could not bind socket\n");
		exit(1); 
	} 

	//listen for client request
	if ( (listen(sockFD, 5) < 0) ) {
		fprintf(stderr, "Error: server failed to listen\n"); 
		exit(1); 
	}
	
	printf("Server is listening on port %d...\n", portNumber); 

	sizeOfClientInfo = sizeof(clientAddress); //absolutely needed! to get server to read message

	if ( (connectedSockFD = accept(sockFD, (struct sockaddr *)&clientAddress, &sizeOfClientInfo)) < 0) {
		fprintf(stderr, "Error: server failed to accept client address\n"); 	
	}
/*
	socklen_t len; 
	struct sockaddr_storage addr; 
	//char hostname[128];
	struct in_addr* hostname; 
	memset(hostname, '\0', 128); 
	len = sizeof(addr); 

	getpeername(connectedSockFD, (struct sockaddr*)&addr, &len); 
	struct sockaddr_in *s = (struct sockaddr_in*)&addr; 

	inet_ntop(AF_INET, &s->sin_addr, hostname, sizeof(hostname)); 
*/

	//serverHostInfo = gethostbyaddr( (struct in_addr*)&s->sin_addr, sizeof(s->sin_addr), AF_INET); 
	serverHostInfo = gethostbyaddr( (struct in_addr*)&clientAddress.sin_addr, sizeof(clientAddress.sin_addr), AF_INET); 
	printf("%s\n", serverHostInfo->h_name);  

//	getpeername(connectedSockFD, (struct sockaddr*)&clientAddress, &sizeOfClientInfo);
//	printf("%s\n", clientAddress);  
	return connectedSockFD; 
}

int startDataConnection(char* dataPort, char* hostName) {
	int sockFD, dataPortNumber;
	struct sockaddr_in clientAddress;
	struct hostent* clientHostInfo;
	memset( (char*)&clientAddress, '\0', sizeof(clientAddress)); 
	dataPortNumber = atoi(dataPort);

	clientAddress.sin_family = AF_INET;
	clientAddress.sin_port = htons(dataPortNumber);

	
	printf("data connection started\n"); 
/*	int ret;
	char buffer[100]; 	
	gethostname(buffer, sizeof(buffer)); 
	//if(clientHostInfo == NULL) {
	printf("host is: %s\n", buffer); 
	if(buffer == NULL) {
		fprintf(stderr, "Server error: no such client host\n"); 
		//exit(1)		
	}
*/
	if ( (clientHostInfo = gethostbyname(hostName)) == NULL) {
		fprintf(stderr, "Server error: cannot find client host\n"); 
		exit(1); 
	}

	memcpy( (char*)&clientAddress.sin_addr.s_addr, (char*)clientHostInfo->h_addr, clientHostInfo->h_length); 

	if((sockFD = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		fprintf(stderr, "Server error: failed to create socket\n"); 
		//exit(1); 
	}
	if(connect(sockFD, (struct sockaddr*)&clientAddress, sizeof(clientAddress)) < 0) {
		fprintf(stderr, "Server error: failed to connect socket\n"); 
	}
	
	printf("data connection successful\n"); 
	return sockFD; 

}

	
//receive message
void recvMsg(int sockFD, char* msg, int size, int flag) {
	int numOfBytes;

	//SHOULD I CHECK THE SIZE OF BYTES RECEIVED HERE??	
	if ((numOfBytes = recv(sockFD, msg, size, flag) < 0) ) {
		fprintf(stderr, "Error: server failed to receive client msg\n"); 	
		exit(1); 
	}
}


void sendMsg(int sockFD, const void* msg, int size, int flag) {
	int bytesSent; 	

	if ( (bytesSent = send(sockFD, msg, size, flag) < 0) ) {
		fprintf(stderr, "Error: server failed to send client the msg\n"); 
		exit(1); 
	}

} 

//deal with command
void handleCommand(int sockFD, char* clientCommand, char* clientPortNum) {
	
	int dataSocketFD; 
	DIR* dir; 
	struct dirent* p; 
//	memset(p->d_name, '\0', sizeof(p->d_name)); 
	dir = opendir("."); 
	
	p=readdir(dir); 
	p=readdir(dir);
	 	
	//parse; if str doesn't begin with '-l ' or '-g ' return error
	if ( (strcmp(clientCommand, "-l") != 0) && (strcmp(clientCommand, "-g") != 0)) {
		sendMsg(sockFD, "ERROR", 7, 0);
	}

	else {
		sendMsg(sockFD, "LISTEN", 7, 0); 
		
		struct sockaddr_in addr; 
		socklen_t addr_len = sizeof(addr); 
		struct hostent* clientHostInfo;
		
		if (getpeername(sockFD, (struct sockaddr*) &addr, &addr_len) == -1) {
			fprintf(stderr, "server failed to get client name\n"); 	
		}
		
		printf("hello\n");  
	//	printf("hello %d\n", inetaddr.sin_addr.s_addr); 
		
		//printf("%d\n", addr.sin_addr.s_addr); 
		//printf("%d\n", inet_ntop(addr.sin_addr.s_addr)); 
		clientHostInfo = gethostbyaddr( (struct in_addr*)&addr.sin_addr, sizeof(addr.sin_addr), AF_INET); 
		printf("%s\n", clientHostInfo->h_name);
		char* hostName = clientHostInfo->h_name; 
		dataSocketFD = startDataConnection(clientPortNum, hostName); 

		if (strcmp(clientCommand, "-l") == 0) {
			char directoryStr[512];
			memset(directoryStr, '\0', sizeof(directoryStr)); 
			sendMsg(dataSocketFD, "ACK_l", 7, 0); 
			while ( ((p = readdir(dir)) != NULL) ) {
				strcat(directoryStr, p->d_name); 
				strcat(directoryStr, " "); 
			}
			sendMsg(dataSocketFD, directoryStr, 512, 0);
			
			close(dataSocketFD); 
		}

		else if ( strcmp( strtok(clientCommand, " "), "-g") == 0) {
			char* fileName = strtok(NULL, " "); 
			sendMsg(dataSocketFD, "ACK_g", 7, 0); 

			if (fileName == NULL) {
				sendMsg(sockFD, "ERROR", 7, 0); 	
			}
		}
/*		

*/
	}




/*

	else if ( strcmp( strtok(clientCommand, " "), "-g") == 0) {
		//printf("tokenization successful\n"); 
		
		char* fileName = strtok(NULL, " "); 

		if (fileName == NULL) {
			sendMsg(sockFD, "ERROR", 7, 0); 	
		}
		else {	
			while ( ((p = readdir(dir)) != NULL) ) {
				if (strcmp(p->d_name, fileName) == 0) {
					printf("file exists\n"); 	
					sendMsg(sockFD, "ACK_g", 7, 0);	
					//reset fileName
					

					//set flag or establish TCP data connection 

 
				}
			}	 
		}
		printf("file doesn't exist\n"); 
		sendMsg(sockFD, "FILENF", 7, 0);
	}
				//sendMsg(sockFD, 
	//if str is '-l ' 
		//do this

	//else str is '-g '
		//check the filename is valid
	
	memset(clientCommand, '\0', sizeof(clientCommand)); 
*/


}

int main(int argc, char* argv[]) {

	//validate commandline parameters
	if (argc != 2) {
		fprintf(stderr, "Usage: %s [port number]\n", argv[0]);
		exit(1);
	}	
	
	int socketFD;	
	char clientPort[256], clientCMD[256]; 
	memset(clientPort, '\0', 256); 	
	memset(clientCMD, '\0', 256); 	

	//start connection and listen
	socketFD = startConnection(argv[1]);
	
	//receive client's dataport
	recvMsg(socketFD, clientPort, sizeof(clientPort), 0); 

	//server must continue listening even after client disconnects
	while (1) {
		recvMsg(socketFD, clientCMD, sizeof(clientCMD), 0); 
		printf("Server received: %s\n", clientCMD); 
	
		//parse client cmd
		handleCommand(socketFD, clientCMD, clientPort); 	
		//if wrong, send error message
		//if right, send an ACK like a '1' so client can start listening
		//then send directory or file 

		
	}


	return 0;
}
