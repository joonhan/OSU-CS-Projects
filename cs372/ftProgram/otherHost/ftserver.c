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
#include <stdbool.h>
#include <fcntl.h>
#include <limits.h>
#define CMD_STR_LEN 256
#define PORT_STR_LEN 256
#define FILESIZE 65536 
#define MIN(a,b) a<b?a:b


/* TO DOs
 * 1) handling duplicate file names
 * 2) SIGINT issues
 * 3) Refactor code, especially sending of files
 * 3) adding features - cd, put, listing local directory, etc
*/



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

int startDataConnection(char* hostName, char* dataPort) {
	int dataSockFD, dataPortNumber;
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

	if((dataSockFD = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		fprintf(stderr, "Server error: failed to create socket\n"); 
		//exit(1); 
	}
	if(connect(dataSockFD, (struct sockaddr*)&clientAddress, sizeof(clientAddress)) < 0) {
		fprintf(stderr, "Server error: failed to connect socket\n"); 
	}
	
	printf("data connection successful\n"); 
	return dataSockFD; 

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

//send message
void sendMsg(int sockFD, const void* msg, int size, int flag) {

	int bytesSent; 	
	if ( (bytesSent = send(sockFD, msg, size, flag) < 0) ) {
		fprintf(stderr, "Error: server failed to send client the msg\n"); 
		exit(1); 
	}
/*
	int bytesLeft = size;
	int bytesSent = 0 ;

	while (bytesLeft != 0) { 	
		bytesSent = send(sockFD, msg, MIN(bytesLeft, 65536), flag); 
		printf("bytes sent: %d\n", bytesSent); 
		if (bytesSent < 0) {
			fprintf(stderr, "Error: server failed to send client the msg\n"); 
			exit(1); 
		}
		if (bytesSent == 0) {
			break;
		}
		if (bytesSent > 0) {
			bytesLeft = bytesLeft - bytesSent;	
			printf("bytes left: %d\n", bytesLeft); 
		}
	}
	*/
} 


char* getClientName(int sockFD) {
	/////////////////////// move this into startDataConnection()	
	struct sockaddr_in addr; 
	socklen_t addr_len = sizeof(addr); 
	struct hostent* clientHostInfo;
	char* hostName; 	

	if (getpeername(sockFD, (struct sockaddr*) &addr, &addr_len) == -1) {
		fprintf(stderr, "server failed to get client name\n"); 	
	}
		
	clientHostInfo = gethostbyaddr( (struct in_addr*)&addr.sin_addr, sizeof(addr.sin_addr), AF_INET); 
	printf("server got client name: %s\n", clientHostInfo->h_name);
	hostName = clientHostInfo->h_name; 

	return hostName; 
}

void setDirectory(char* directoryStr) {
	DIR* dir; 
	struct dirent* p;
	//char directoryStr[512];
	//memset(directoryStr, '\0', sizeof(directoryStr)); 

	dir = opendir("."); 
	p=readdir(dir); 
	p=readdir(dir);
	 	
	while ( ((p = readdir(dir)) != NULL) ) {
		strcat(directoryStr, p->d_name); 
		strcat(directoryStr, " "); 
	}

	//return directoryStr; 
}

bool fileExists(char* fileName) {
	DIR* dir2;
	struct dirent* q; 
	dir2 = opendir("."); 
	while ( ((q = readdir(dir2)) != NULL) ) {
		if (strcmp(q->d_name, fileName) == 0) {
	 	 	printf("file exists, returning true\n");
			return true;	
		}
	}
	return false; 
}


char* getFile(char* fileName) {
	char *file;
	int bytes;
	//FILE* fp;
	int fd; 

	char filepath[PATH_MAX+1]; 
//	if (  (fp = fopen(fileName, "r")) == NULL ) {
	if ( fd = open(filepath, O_RDONLY) < 0) {
		fprintf(stderr, "Can't open file\n"); 
		return NULL; 	
	} 
	 
	bytes = read(fd, file, sizeof(file));
	
	

	return file;  
}






//deal with command
void handleCommand(int sockFD, char* clientCommand, char* clientPortNum) {
	
	int dataSockFD; 
	//DIR* dir; 
	//struct dirent* p;
	char* hostName;
	char* fileName;
	char directoryStr[512];
	memset(directoryStr, '\0', sizeof(directoryStr)); 
//	memset(p->d_name, '\0', sizeof(p->d_name)); 
	//dir = opendir("."); 
	
	//p=readdir(dir); 
	//p=readdir(dir);
	 	
	//parse; if str doesn't begin with '-l ' or '-g ' return error
	if ( (strcmp(clientCommand, "-l") != 0) && (strcmp( strtok(clientCommand, " "), "-g") != 0)) {
		sendMsg(sockFD, "ERROR", 7, 0);
	}

	/*if ( (strcmp(clientCommand, "-g") == 0)) {
		char* fileName = 
	}	*/

	else {
		sendMsg(sockFD, "LISTEN", 7, 0); 

		hostName = getClientName(sockFD); 
		dataSockFD = startDataConnection(hostName, clientPortNum); 

		if (strcmp(clientCommand, "-l") == 0) {
			sendMsg(sockFD, "ACK_l", 7, 0); 
			
			//directoryStr = getDirectory();
			setDirectory(directoryStr);

			//sendMsg(dataSockFD, directoryZ, 512, 0);
			sendMsg(dataSockFD, directoryStr, 512, 0);			///improvement: send variable size to client
			close(dataSockFD); 
		}

		//else if ( strcmp( strtok(clientCommand, " "), "-g") == 0) {
		else {
			sendMsg(sockFD, "ACK_g", 7, 0); 
			
			fileName = strtok(NULL, " "); 
			printf("fileName is: %s\n", fileName); 	

			if (fileName != NULL) {
				printf("checking file\n"); 

				if (fileExists(fileName) == 1) {
					printf("sending file\n"); 
					sendMsg(sockFD, "FILEIN", 7, 0); 

					struct stat fs; 

					FILE *fp; 

					char file[6488394]; 
					memset(file, '\0', sizeof(file)); 
					int fd = open(fileName, O_RDONLY); 
					if (fd < 0) {
						fprintf(stderr, "Could not open file\n"); 
					}

					fstat(fd, &fs);
					printf("%d\n", fs.st_size);
					//int x = (int) fs.st_size; 
					//printf("%d\n", x); 
					
					int x = 20;
					char size[16];
					memset(size, '\0', sizeof(size)); 
					//sprintf(size, "%d", fs.st_size); 
					sprintf(size, "%d", fs.st_size);  
					printf("size is %s\n", size); 
					sendMsg(dataSockFD, size, 256, 0); 
					
					//use my 'sendFile()' that loops the read and send fxns
					int bytesRead = 0; 
					int totalBytes = fs.st_size; 
					int totalRead = 0; 

					while (totalRead < totalBytes) {
						bytesRead = pread(fd, file, fs.st_size, 0); 
						if (bytesRead < 0) {
							fprintf(stderr, "Error sending file\n"); 	
						}	

						totalRead += bytesRead; 
						printf("total read: %d\n", totalRead); 
					} 
					int fd2 = open("result.txt", O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP); 
					if (fd2 < 0) fprintf(stderr, "failed to write\n"); 
					write(fd2, file, fs.st_size); 				
					close(fd2);
					/*FILE* fp2;
					fp2 = fopen("result.txt", "w"); 
					if (fp2 < 0) fprintf(stderr, "failed to write\n"); 
					fprintf(fp2, file);
					fclose(fp2); 	*/			
			
					printf("Sending THE file\n"); 
					//sendMsg(dataSockFD, file, 6488394, 0); 
					sendMsg(dataSockFD, file, fs.st_size, 0); 
					
					close(dataSockFD); 
					
				}
				else {
					printf("file not found in here\n"); 
					//file not found
					sendMsg(sockFD, "FILENF", 7, 0);
					close(dataSockFD); 
				}

			}
			else {
				sendMsg(sockFD, "FILENG", 7, 0);	
				printf("FILENG sent\n"); 
				close(dataSockFD); 
			}
		}
	}
		
	printf("resetting cmd\n"); 
	memset(clientCommand, '\0', CMD_STR_LEN); 
	printf("clientcommand is: %s\n", clientCommand); 
	//memset(fileName, '\0', sizeof(fileName)); 
}	//end handleCommand()

void handleTransfer(int controlSockFD) {

	char clientPort[PORT_STR_LEN], clientCMD[CMD_STR_LEN]; 
	memset(clientPort, '\0', PORT_STR_LEN); 	
	memset(clientCMD, '\0', CMD_STR_LEN); 	

	recvMsg(controlSockFD, clientPort, sizeof(clientPort), 0); 

	while (1) {
		recvMsg(controlSockFD, clientCMD, sizeof(clientCMD), 0); 
		printf("Server received: %s\n", clientCMD); 

		handleCommand(controlSockFD, clientCMD, clientPort); 	
	}
}



int main(int argc, char* argv[]) {

	//validate commandline parameters
	if (argc != 2) {
		fprintf(stderr, "Usage: %s [port number]\n", argv[0]);
		exit(1);
	}	
	
	int socketFD;	
/*
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

*/
	socketFD = startConnection(argv[1]); 
	
	handleTransfer(socketFD); 



	return 0;
}
