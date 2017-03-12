/* CS372 Winter2017
 * Joon Han
 * Program 2, ftserver.c
 * Server side of a simple file transfer system. 
 * Note: Parts of this code uses code I wrote for Program 1, which references heavily from 
 * Beej's Guide to Network Programming. 
 */

/* Extra Credit: Added capability to change directory using '-cd [path]' */

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
#include <signal.h>

#define CMD_STR_LEN 256
#define PORT_STR_LEN 256
#define RESPONSE_LEN 7
#define DIR_STR_LEN 512
#define START_DIR_LEN 512
#define FILESIZE 65536 
#define MIN(a,b) a<b?a:b


/* Function: startServer
 *	---------------------
 * purpose: create and bind a server socket
 * parameters: argv1 is the server port number cmd line arg
 * returns: the socket file descriptor
 */
int startServer(char* argv1) {

	int sockFD, connectedSockFD, portNumber; 

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
 	int optVal = 1;
   if (setsockopt(sockFD, SOL_SOCKET, SO_REUSEADDR, &optVal, sizeof(optVal)) < 0) {
       fprintf(stderr, "Error: server attempt to reuse socket failed\n");
       exit(1);
   }	
	
	//bind the address
	if (bind(sockFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
		fprintf(stderr, "Error: server could not bind socket\n");
		exit(1); 
	} 

	return sockFD; 
}


/* Function: startDataConnection 
 *	-----------------------------
 * purpose: request and connect to the client to establish data connection
 * parameters: hostName is the name of client host; dataPort is client's listening port
 * returns: socket file descriptor for the data connection
 */
int startDataConnection(char* hostName, char* dataPort) {
	int dataSockFD, dataPortNumber;
	struct sockaddr_in clientAddress;
	struct hostent* clientHostInfo;
	memset( (char*)&clientAddress, '\0', sizeof(clientAddress)); 
	dataPortNumber = atoi(dataPort);

	clientAddress.sin_family = AF_INET;
	clientAddress.sin_port = htons(dataPortNumber);

	//retrieve's host's IP address from given name
	if ( (clientHostInfo = gethostbyname(hostName)) == NULL) {
		fprintf(stderr, "Error: server cannot find client host\n"); 
		exit(1); 
	}

	//copy client info into socket struct
	memcpy( (char*)&clientAddress.sin_addr.s_addr, (char*)clientHostInfo->h_addr, clientHostInfo->h_length); 

	//create socket
	if((dataSockFD = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		fprintf(stderr, "Error: server failed to create data socket\n"); 
		//exit(1); 
	}

	//connect to client 
	if(connect(dataSockFD, (struct sockaddr*)&clientAddress, sizeof(clientAddress)) < 0) {
		fprintf(stderr, "Error: server failed to connect data socket\n"); 
	}
	
	return dataSockFD; 
}

	
/* Function: recvMsg 
 *	-----------------
 * purpose: receive message sent by client
 * parameters: sockFD is active socket; msg is buffer to store data; msgSize is size of message; flag is 
 * for any flag options
 * returns: N/A
 */
void recvMsg(int sockFD, char* msg, int msgSize, int flag) {
	int numOfBytes;

	if ((numOfBytes = recv(sockFD, msg, msgSize, flag) < 0) ) {
		fprintf(stderr, "Error: server failed to receive client msg\n"); 	
		exit(1); 
	}
}


/* Function: sendMsg 
 *	-----------------
 * purpose: sent message to client; data is sent in chunks until complete 
 * parameters: sockFD is active socket; msg is ptr to data; msgSize is size of message; flag is 
 * for any flag options
 * returns: N/A
 */
void sendMsg(int sockFD, const void* msg, int msgSize, int flag) {
	int bytesLeft = msgSize;
	int bytesSent = 0 ;
	int offset = 0; 

	while (bytesLeft != 0) { 	
		//offset added to msg ptr; w/o offset, send() will send from beginning of 'msg' for large msgs
		bytesSent = send(sockFD, msg + offset, MIN(bytesLeft, FILESIZE), flag); 

		//error occurred
		if (bytesSent < 0) {
			fprintf(stderr, "Error: server failed to send msg to client\n"); 
			exit(1); 
		}
		//sending complete; exit loop
		if (bytesSent == 0) {
			break;
		}
		//decrement amount of data sent from total
		if (bytesSent > 0) {
			bytesLeft = bytesLeft - bytesSent;	
		}
		offset += bytesSent; 
	}
} 


/* Function: getClientName 
 *	-----------------------
 * purpose: retrieve name of connected client 
 * parameters: sockFD refers to socket where connection to client exists
 * returns: string representing client's name
 */
char* getClientName(int sockFD) {
	struct sockaddr_in addr; 
	socklen_t addr_len = sizeof(addr); 
	struct hostent* clientHostInfo;
	char* hostName; 	

	//get info about client connected to socket and store it in 'addr'
	if (getpeername(sockFD, (struct sockaddr*) &addr, &addr_len) == -1) {
		fprintf(stderr, "Error: server failed to get client name\n"); 
		exit(1); 	
	}
	
	//get host's name using it's IP address
	clientHostInfo = gethostbyaddr( (struct in_addr*)&addr.sin_addr, sizeof(addr.sin_addr), AF_INET); 
	hostName = clientHostInfo->h_name; 

	return hostName; 
}


/* Function: setDirectory 
 *	----------------------
 * purpose: retrieve directory of files and assign it as a string to ptr in parameter
 * parameters: directoryStr is the buffer to store the directory info in string form
 * returns: N/A
 */
void setDirectory(char* directoryStr) {
	//create pointer to directory stream and directory entry struct
	DIR* dir; 
	struct dirent* p;

	//open current directory
	dir = opendir("."); 

	//shift the pointer past the first entries ('.' and '..') 	
	p=readdir(dir); 
	p=readdir(dir);
	 
	//while directory entry exists, concatentate into string	
	while ( ((p = readdir(dir)) != NULL) ) {
		strcat(directoryStr, p->d_name); 
		strcat(directoryStr, " "); 
	}
}


/* Function: fileExists 
 *	--------------------
 * purpose: determine whether a file exists in the directory 
 * parameters: fileName is name of file (specified by client host)
 * returns: bool value indicating whether file exists in current directory 
 */
bool fileExists(char* fileName) {
	DIR* dir2;
	struct dirent* q;
	
	//open current directory 
	dir2 = opendir("."); 
	
	//if fileName matches a file in directory, return true
	while ( ((q = readdir(dir2)) != NULL) ) {
		if (strcmp(q->d_name, fileName) == 0) {
			return true;	
		}
	}
	return false; 
}


/* Function: sendFile 
 *	--------------------
 * purpose: opens file, saves it in buffer, then sends it to client
 * parameters: dataSockFD refers to socket used to transmit file; fileName is name of file in directory
 * returns: N/A
 */
void sendFile(int dataSockFD, char *fileName) {
	struct stat fs; 		//struct to store file stats
	int fd;

	//open file
	if ( (fd = open(fileName, O_RDONLY)) < 0) {
		fprintf(stderr, "Error: could not open file\n");
		exit(1); 
	}

	//get file stats from open file 
	fstat(fd, &fs);

	//create a buffer as large as the open file to store file contents
	char file[fs.st_size]; 
	memset(file, '\0', sizeof(file)); 

	//create a buffer to store the size of file as a string
 	char size[256];
	memset(size, '\0', sizeof(size)); 

	//convert the number value of file size to a string
	sprintf(size, "%d", fs.st_size);  

	//send the size of file to client as string
	sendMsg(dataSockFD, size, 256, 0); 
	
	//read file contents into buffer					
	if ( (read(fd, file, fs.st_size)) < 0) {
		fprintf(stderr, "Error: could not read file\n");
		exit(1); 
	} 

	sendMsg(dataSockFD, file, fs.st_size, 0); 
}


////////////////////////////////////////////
void handleSIGINT(int signo) {
	if (signo == SIGINT) {
		printf("Server interrupted. Shutting down.\n"); 
		exit(1); 
	}
}


/* Function: handleCommand 
 *	-----------------------
 * purpose: receives commands from client and handles  
 * parameters: ctrlSockFD refers to the control connection established with client
 * returns: N/A
 */
//void handleCommand(int ctrlSockFD, char* clientCommand, char* clientPortNum) {
void handleCommand(int ctrlSockFD) {

	int dataSockFD; 
	char* hostName;
	char* fileName;
	char* dirPath; 
	char curDir[512];
	DIR* dir;
	char directoryStr[DIR_STR_LEN], clientPortNum[PORT_STR_LEN], clientCommand[CMD_STR_LEN]; 
	memset(clientPortNum, '\0', PORT_STR_LEN); 	

/*	struct sigaction sa; 
	sa.sa_handler = SIGINT;	
	sigaction(SIGINT, &sa, handleSIGINT); 
*/

	//receive client's port number (used to establish data connection)
	recvMsg(ctrlSockFD, clientPortNum, sizeof(clientPortNum), 0); 

	//loop until client sends 'quit'
	while (1) {
		//reset command & directory strings prior to each loop
		memset(clientCommand, '\0', CMD_STR_LEN); 
		memset(directoryStr, '\0', sizeof(directoryStr)); 

		//receive client's command
		recvMsg(ctrlSockFD, clientCommand, sizeof(clientCommand), 0); 

		//server receives quit msg 
		if (strcmp(clientCommand, "quit") == 0) {
			printf("Client has disconnected.\n"); 
			close(ctrlSockFD); 
			break;
		}	
		//Extra credit: change directory features	
		else if ( strncmp(clientCommand, "-cd", 3) == 0) {
			//ignore the '-cd' portion and extract path argument
			strtok(clientCommand, " "); 
			dirPath = strtok(NULL, " "); 

			getcwd(curDir, sizeof(curDir)); 

			//path arg is not provided; if not at HOME go home 
			if ( (dirPath == NULL) ) {
				if ((strcmp(getenv("HOME"), curDir) != 0)) {
					chdir(getenv("HOME")); 
				}
				else {
					sendMsg(ctrlSockFD, "ENDDIR", RESPONSE_LEN, 0); 	
					continue;	
				}
			}
			//path arg given but invalid.. can't open dir
			else if ( (dir = opendir(dirPath)) == NULL ) {
				fprintf(stderr, "Error: server could not find directory\n"); 
				sendMsg(ctrlSockFD, "BADDIR", RESPONSE_LEN, 0); 	
				continue;
			}		

			//if at HOME and attempting to go back further, stay in same dir
			else if ( (strcmp(getenv("HOME"), curDir) == 0) && (strcmp(dirPath, "..") == 0) ) {
				sendMsg(ctrlSockFD, "ENDDIR", RESPONSE_LEN, 0); 	
				continue;
			}	
			//otherwise valid path was given; change dir
			else { 
				chdir(dirPath); 
			}	
			sendMsg(ctrlSockFD, "CHGDIR", RESPONSE_LEN, 0); 	
			getcwd(curDir, sizeof(curDir));
			printf("Changed directory to %s\n", curDir);
			//if (dir != NULL) closedir(dir); 
		}
		//** anything other than '-l' or '-g' is considered invalid.. for the purpose of project

		//client cmd is neither '-l' nor begin with '-g'	Note: strtok alters clientCommand!!
		else if ( (strcmp(clientCommand, "-l") != 0) && (strcmp( strtok(clientCommand, " "), "-g") != 0)) {
			sendMsg(ctrlSockFD, "ERROR", RESPONSE_LEN, 0);
		}

		//direct client to begin listening
		else {
			sendMsg(ctrlSockFD, "LISTEN", RESPONSE_LEN, 0); 

			//initialize data connection 
			hostName = getClientName(ctrlSockFD); 
			dataSockFD = startDataConnection(hostName, clientPortNum); 

			//client sent '-l' over data connection
			if (strcmp(clientCommand, "-l") == 0) {
				//acknowledge to client that '-l' was received; save directory to string and send
				sendMsg(ctrlSockFD, "ACK_l", RESPONSE_LEN, 0); 
		
				printf("Client requested directory listing\n"); 	
				setDirectory(directoryStr);
				
				printf("Sending directory listing on port %s\n", clientPortNum); 	
				sendMsg(dataSockFD, directoryStr, DIR_STR_LEN, 0);	//can improve to send variable length		
			}

			//client command started with '-g'
			else {
				//acknowledge to client that '-g' was received and read filename after '-g'
				sendMsg(ctrlSockFD, "ACK_g", RESPONSE_LEN, 0); 
				fileName = strtok(NULL, " "); 

				//if a file name is given, check whether file exists in directory
				if (fileName != NULL) {
				
					printf("Client requested file \'%s\'\n", fileName); 	

					//acknowledge that file was found and send file
					if (fileExists(fileName) == 1) {
						sendMsg(ctrlSockFD, "FILEIN", RESPONSE_LEN, 0); 
						printf("Sending \'%s\' on port %s\n", fileName, clientPortNum); 	
						sendFile(dataSockFD, fileName); 
					}
					//send file not found message
					else {
						sendMsg(ctrlSockFD, "FILENF", RESPONSE_LEN, 0);
						printf("File not found\n"); 
					}
				}
				//file name was not given
				else {
					sendMsg(ctrlSockFD, "FILENG", RESPONSE_LEN, 0);	
					printf("File not given\n"); 
				}
			}
			close(dataSockFD);
		}
	} //end of while loop
}	//end of handleCommand()


/* Function: connectClient 
 *	-----------------------
 * purpose: listen for and connect to incoming client connection 
 * parameters: serverSockFD is server's socket to listen on; port is the port number from cmd line arg 
 * returns: socket connection established with client
 */
int connectClient(int serverSockFD, char* port) {
	int ctrlSockFD, portNumber; 
	socklen_t sizeOfClientInfo; 
	struct sockaddr_in clientAddress; 
	portNumber = atoi(port); 

	memset( (char*)&clientAddress, '\0', sizeof(clientAddress)); 

	//listen for client connection	
	if ( (listen(serverSockFD, 5) < 0) ) {
		fprintf(stderr, "Error: server failed to listen\n"); 
		exit(1); 
	}
	
	printf("Server is listening on port %d...\n", portNumber); 
	sizeOfClientInfo = sizeof(clientAddress); //required to get server to receive request 

	//accept client connection request
	if ((ctrlSockFD = accept(serverSockFD, (struct sockaddr *)&clientAddress, &sizeOfClientInfo)) < 0) {
		fprintf(stderr, "Error: server failed to accept client address\n"); 	
	}
	
	return ctrlSockFD; 
}


int main(int argc, char* argv[]) {
	//validate commandline parameters
	if (argc != 2) {
		fprintf(stderr, "Usage: %s [port number]\n", argv[0]);
		exit(1);
	}	
	
	int serverSockFD, ctrlSockFD;	
	serverSockFD = startServer(argv[1]); 

	//using '-cd' and chdir() will cause server directory to change and new client connections
	//will stay in last changed directory. 
	//thus save starting directory and reset directory when new a client connects
	char startDir[START_DIR_LEN]; 
	getcwd(startDir, sizeof(startDir)); 

	//continue loop even after client disconnects
	while (1) {
		ctrlSockFD = connectClient(serverSockFD, argv[1]); 
		handleCommand(ctrlSockFD);
		chdir(startDir); 		 		//reset directory for new connection	
	}

	return 0;
}
