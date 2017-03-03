## CS372 Winter2017
## Joon Han
## Program 1, chatserve.py
## This program runs the 'server' side of a simple chat program. 

from socket import *
import argparse
import sys 
import signal 


#signal handler for interrupt signals
def sigIntHandler(signum, frame):
	print "\nYou pressed Ctrl+C. Exiting program..."
	sys.exit(0)


#initiate server socket
def startServer(serverPort): 
	serverSocket = socket(AF_INET, SOCK_STREAM)
	serverSocket.bind(('', serverPort))
	serverSocket.listen(1)

	return serverSocket


#send a message via given socket and verify all of it has been sent
#code referenced from https://docs.python.org/3/howto/sockets.html
def sendMsg(connectedSocket, msg): 
	
	#set total bytes sent as 0
	totalBytes = 0
	
	#while bytes sent is less than msg length, keep sending and add sent bytes to total 
	while totalBytes < len(msg): 
		sentBytes = connectedSocket.send(msg)
		if sentBytes == 0:
			raise RuntimeError("Socket connection is broken")	
		totalBytes = totalBytes + sentBytes


#receive a message via given socket and verify that all data is received
#code referenced from https://docs.python.org/3/howto/sockets.html
def recvMsg(connectedSocket, msgSize):
	#totalData stores each received message (in pieces) into a list
	totalData = []
	totalBytes = 0

	#while the bytes received is less than size of message
	while totalBytes < msgSize: 
		#recv as data an amount that is the min between (original-recvd) or set value
		dataReceived = connectedSocket.recv(min(msgSize-totalBytes, 2048))
	
		#if no data is received	return empty string
		if dataReceived == '': 
			return ''

		#otherwise append the data to the existing data list
		totalData.append(dataReceived)
				
		#keep a running sum of bytes received
		totalBytes = totalBytes + len(dataReceived)		
	
	#join the list elements into a string ('' is no space between them)
	return ''.join(totalData) 


#run main program loop
def chatStart(connectedSocket): 
	
	#loop forever (until '\quit' is entered) 
	while 1: 
		
		#receive initial client message
		clientName = recvMsg(connectedSocket, 64)
		
		if(clientName == ''):
			print "Connection lost. Restarting."
			break 
		
		#display client name prompt without a newline
		sys.stdout.write(clientName + '> ')

		#clear the name after printing it
		clientName = ''; 

		#receive incoming message up 500 chars
		#need a minimum of 41 bytes (TCP header is 20bytes and IPV4 header is 20bytes?)
		incomingMsg = recvMsg(connectedSocket, 500)

		print incomingMsg

		#read input message	
		message = raw_input('server> ')

		#if user enters '\quit', close connection	
		if (message == '\quit'): 
			connectedSocket.shutdown(SHUT_WR); 
			connectedSocket.close();
			print "Connection closed." 
			break 
		
		#send the server handle and message	
		sendMsg(connectedSocket, 'server'); 
		sendMsg(connectedSocket, message); 
		

# main function
if __name__ == "__main__": 
	
	#use a parser to parse cmd line arguments	
	parser = argparse.ArgumentParser()
	parser.add_argument("port", type=int, help="port number")
	args = parser.parse_args()
	
	#save server port# 
	serverPort = args.port
	
	serverSocket = startServer(serverPort); 	

	#this loop continues to run even after '\quit' is typed; but SIGINT ends it
	while 1: 
		print "Server is listening on port " + str(serverPort) + "..."
		signal.signal(signal.SIGINT, sigIntHandler)
		connectedSocket, addr = serverSocket.accept()

		chatStart(connectedSocket); 
	

