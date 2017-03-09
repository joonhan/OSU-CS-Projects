
import socket
import argparse 
import sys
import signal
import errno
import tempfile
import os
import struct


def initiateConnection(host, serverPort): 
	#attempt to create socket and connect to server
	try: 
		controlSock = socket.socket(socket.AF_INET, socket.SOCK_STREAM) 
	except socket.error:
		print 'Error: client failed to create socket'
		sys.exit(1)

	try: 
		controlSock.connect((host, serverPort)) 
	except socket.error:
		print 'Error: client failed to connect to server'
		sys.exit(1)

	return controlSock 
######################################

def sendMsg(connectedSocket, msg): 

	totalBytes = 0
	
	while totalBytes < len(msg): 
		sentBytes = connectedSocket.send(msg) 
		if sentBytes == 0:
			raise RuntimeError("Socket connection is broken")
		totalBytes += sentBytes


def recvMsg(connectedSocket, msgSize): 
	totalData = []
	totalBytes = 0

	while totalBytes < msgSize: 
	
		dataReceived = connectedSocket.recv(min(msgSize-totalBytes, 65536))

		if dataReceived == '':
			return 'no data received'

		totalData.append(dataReceived)
		totalBytes += len(dataReceived)
	
	return ''.join(totalData)	

def startListen(dataPort): 
	dataSocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
	dataSocket.bind(('', dataPort))
	dataSocket.listen(1)	
	print 'Client is listening on port ' + str(dataPort)

	dataSocket, addr = dataSocket.accept()	

	return dataSocket	
########

def printDirectory(directoryStr): 
	directoryStr = directoryStr.rstrip('\x00')
	directoryList = directoryStr.split(" ")
	directoryList = filter(None, directoryList)
	directoryList.sort()

	for fileName in directoryList: 
		sys.stdout.write(fileName + '  ')
	print ''

def processFileName(clientInput): 
	
	fileName = clientInput[3:]

	tail = 1
	while os.path.isfile(fileName):

		periodIndex = fileName.rindex('.')
	
		if periodIndex == -1: 	
			print 'period doesn\'t exist'
			newFileName = fileName + str(tail)
			if not os.path.isfile(newFileName):
				fileName = newFileName

		else: 
			print 'period exists'
			newFileName = fileName[:periodIndex] + str(tail) + fileName[periodIndex:]

			if not os.path.isfile(newFileName):
				fileName = newFileName

		tail += 1

	return fileName

def handleTransfer(controlSocket, dataPort): 
	while (1): 
		sys.stdout.write('> ') 	
		clientInput = raw_input()
		if (clientInput == ''): 
			continue
		if (clientInput == 'quit'): 
			#controlSocket.shutdown(SHUT_WR)
			sendMsg(controlSocket, clientInput)
			controlSocket.close()
			print 'Control connection closed'
			break
	
		sendMsg(controlSocket, clientInput)

		#wait for server to validate and send a response.. 
		serverResponse = recvMsg(controlSocket, 7)

		if serverResponse[:5] == 'ERROR':  
			print 'Error received. Try again.' 

		elif serverResponse[:6] == 'LISTEN':
			#print 'Start listen'	
			dataSocket = startListen(dataPort) 

			serverResponse = recvMsg(controlSocket, 7)

			if serverResponse[:5] == 'ACK_l': 
				#print 'ACK_l received'
				dirStr = recvMsg(dataSocket, 512) 
				printDirectory(dirStr)
				dataSocket.close()
			
			elif serverResponse[:5] == 'ACK_g':
				print 'ACK_g received'

				serverResponse = recvMsg(controlSocket, 7)
				print 'serverResponse is ' + serverResponse

				if serverResponse[:6] == 'FILENF': 
					print 'FILE NOT FOUND' 	
					dataSocket.close()

				elif serverResponse[:6] == 'FILENG':
					print 'Error: No file given'
					dataSocket.close()
				
				elif serverResponse[:6] == 'FILEIN': 
					print 'Receiving file'
					fileSize = recvMsg(dataSocket, 256)
					fileSize = fileSize.rstrip('\x00')

					#fileSize = int(fileSize)
					print 'received filesize is ' + fileSize	
					fs = int(fileSize)
					print fs 
					#fileSize = socket.ntohs(fileSize)

					file1 = recvMsg(dataSocket, int(fileSize))
					#filex = file1.rstrip('\x00')

					fileName = processFileName(clientInput)

					print 'fileName is ' + fileName
					f = open(fileName, 'w')
					f.write(file1)
					#with open(fileName, 'a') as f: 
					#	f.write(file1) 	

					f.close()
					print 'file transfer complete'

					dataSocket.close()

	

#main function
if __name__ == "__main__": 
	
	parser = argparse.ArgumentParser(description='File transfer program (client)', 
												usage='python %(prog)s [host] [server port] [data port]')
	parser.add_argument("host")
	parser.add_argument("serverPort", type=int)
	parser.add_argument("dataPort", type=int)
	args = parser.parse_args() 

	#establish TCP control connection
	controlSocket = initiateConnection(args.host, args.serverPort) 
	sendMsg(controlSocket, str(args.dataPort))

	#request = makeRequest(controlConn)
	#sendMsg(controlSocket, request)
	#serverResponse = recv(controlSocket, 7)
	#handleResponse(serverResponse, dataPort)
	
	#VS
		
	handleTransfer(controlSocket, args.dataPort) 


	
