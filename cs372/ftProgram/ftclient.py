"""CS372 Winter2017
	Joon Han
	Program 2, ftclient.py
	Client side of a simple file transfer system. 
	Note: Parts of this code uses code I wrote for Program 1.
"""

import socket
import argparse 
import sys
import signal
import errno
import tempfile
import os
import struct


def sigIntHandler(signum, frame): 
	"""Handle interrupt signals."""	
	print "\nYou pressed ctrl+c. Exiting program..."
	sendMsg(controlSocket, 'quit')
	controlSocket.close()
	sys.exit(0)


def initiateConnection(host, serverPort): 
	"""Create and connect to server then return connected socket."""
	try: 
		controlSock = socket.socket(socket.AF_INET, socket.SOCK_STREAM) 
	except socket.error:
		print 'Error: client failed to create socket'
		sys.exit(1)
	
	#allow reuse of socket in TIME_WAIT state
	controlSock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1); 	

	try: 
		controlSock.connect((host, serverPort)) 
	except socket.error:
		print 'Error: client failed to connect to server'
		sys.exit(1)

	return controlSock 


def sendMsg(connectedSocket, msg): 
	"""Send message over socket and verify all has been sent. 
	Code referenced from https://docs.python.org/3/howto/sockets.html """
	totalBytes = 0
	
	#while bytes sent is < length of message, keep sending and add sent bytes to counter
	while totalBytes < len(msg): 
		sentBytes = connectedSocket.send(msg) 
		if sentBytes == 0:
			raise RuntimeError("Socket connection is broken")
		totalBytes += sentBytes


def recvMsg(connectedSocket, msgSize):
	"""Receive msg as list elements, verify all data is received and return a joined string.
	Code referenced from https://docs.python.org/3/howto/sockets.html """

	#totalData stores each received message into a list
	totalData = []
	totalBytes = 0

	while totalBytes < msgSize: 
		#receive as min amount of data	
		dataReceived = connectedSocket.recv(min(msgSize-totalBytes, 65536))

		#if no data is received, return empty string
		if dataReceived == '':
			return ''

		#append data to existing data list
		totalData.append(dataReceived)
		
		#keep a running sum of bytes received
		totalBytes += len(dataReceived)

	#join the list elements into a string ('' indicates no space between them)	
	return ''.join(totalData)	


def startListen(dataPort): 
	"""Create and listen on socket. Return connected socket."""
	dataSocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
	
	#allow reuse of socket in TIME_WAIT state
	dataSocket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1); 	

	dataSocket.bind(('', dataPort))
	dataSocket.listen(1)	
	#print 'Client is listening on port ' + str(dataPort)

	dataSocket, addr = dataSocket.accept()	

	return dataSocket	


def printDirectory(directoryStr): 
	"""Take input string, strip empty values, and store each space-delimited substring in list. Filter empty
	values, sort list, then print list onto screen."""

	directoryStr = directoryStr.rstrip('\x00')
	directoryList = directoryStr.split(" ")
	directoryList = filter(None, directoryList)
	directoryList.sort()

	#print list elements with spaces b/w them; print a newline at end
	for fileName in directoryList: 
		sys.stdout.write(fileName + '  ')
	print ''


def processFileName(clientInput):
	"""Take input file name and check if file exists in directory. Return unmodified or modified filename."""
	#file name is the substring occurring after 1st 3 chars. Only usuable for 3 char cmd i.e '-l' or '-g'
	fileName = clientInput[3:]

	#assign a number to attach to duplicate name files	
	dupCount = 1
	
	#while file is found in directory
	while os.path.isfile(fileName):

		#find the rightmost index in which period occurs (assuming all files have .extentions)  
		#periodIndex = fileName.rindex('.')

		#period not found; create new file name w/ duplicate indicator at end
		periodIndex = fileName.rfind('.')
		if periodIndex == -1: 	
			newFileName = fileName + str(dupCount)

		#period is found; add duplicate indicator just before dot extension
		else: 
			newFileName = fileName[:periodIndex] + str(dupCount) + fileName[periodIndex:]

		#new file name doesn't exist in directory; overwrite old fileName
		if not os.path.isfile(newFileName):
			fileName = newFileName
			print 'Duplicate file found. File renamed \'' + newFileName + '\''

		dupCount += 1

	return fileName


def handleTransfer(controlSocket, dataPort): 
	"""Read and send user cmds. Handle server response."""
	#send client's port that will be used for data connection
	sendMsg(controlSocket, str(dataPort))

	#loop until user chooses 'quit' or SIGINT is caught
	while (1): 
		signal.signal(signal.SIGINT, sigIntHandler)

		#take user input
		sys.stdout.write('> ') 	
		clientInput = raw_input()

		if (clientInput == ''): 
			continue

		if (clientInput == 'quit'): 
			sendMsg(controlSocket, clientInput)
			controlSocket.close()
			print 'Control connection closed'
			break
	
		sendMsg(controlSocket, clientInput)

		#variables to represent constant values
		RESPONSE_LEN = 7
		DIRECTORY_LEN = 512

		#wait for server to validate and send a response.. 
		serverResponse = recvMsg(controlSocket, RESPONSE_LEN)

		#react accordingly to server response
		if serverResponse[:5] == 'ERROR':  
			print 'Error received. Try again.' 

		#once server sends 'LISTEN' msg, begin listening for connection request
		elif serverResponse[:6] == 'LISTEN':
			dataSocket = startListen(dataPort) 

			#once data connection is made, server will acknowledge '-l' or '=g'
			serverResponse = recvMsg(controlSocket, RESPONSE_LEN)

			if serverResponse[:5] == 'ACK_l': 
				dirStr = recvMsg(dataSocket, DIRECTORY_LEN) 
				printDirectory(dirStr)
			
			elif serverResponse[:5] == 'ACK_g':
				serverResponse = recvMsg(controlSocket, RESPONSE_LEN)

				#after server receives '-g' it can respond in multiple ways
				if serverResponse[:6] == 'FILENF': 
					print 'FILE NOT FOUND' 	

				elif serverResponse[:6] == 'FILENG':
					print 'Error: No file given'
				
				elif serverResponse[:6] == 'FILEIN':
					#receive size of incoming file (in string form) and strip unneeded chars
					fileSizeStr = recvMsg(dataSocket, 256)
					fileSizeStr = fileSizeStr.rstrip('\x00')

					#convert file size string into an int
					fileSize = int(fileSizeStr)

					requestedFile = recvMsg(dataSocket, fileSize)
						
					#check if filename is a duplicate
					fileName = processFileName(clientInput)

					f = open(fileName, 'w')
					f.write(requestedFile)
					f.close()

					print 'Transfer complete. File saved as \'' + fileName + '\' on port ' + str(dataPort)

			dataSocket.close()	

#main function
if __name__ == "__main__": 
	#create parser to parse cmd line arg	
	parser = argparse.ArgumentParser(description='File transfer program (client)', 
												usage='python %(prog)s [host] [server port] [data port]')
	parser.add_argument("host")
	parser.add_argument("serverPort", type=int)
	parser.add_argument("dataPort", type=int)
	args = parser.parse_args() 

	controlSocket = initiateConnection(args.host, args.serverPort) 
	handleTransfer(controlSocket, args.dataPort) 
	
