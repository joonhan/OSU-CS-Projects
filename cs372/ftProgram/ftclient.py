
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
	
		dataReceived = connectedSocket.recv(65536)

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



def handleTransfer(controlSocket, dataPort): 
	while (1): 
		sys.stdout.write('> ') 	
		cmdRequest = raw_input()
		if (cmdRequest == ''): 
			continue
		if (cmdRequest == 'quit'): 
			controlSocket.shutdown(SHUT_WR)
			controlSocket.close()
			print 'Control connection closed'
			break
	
		sendMsg(controlSocket, cmdRequest)

		#wait for server to validate and send a response.. 
		cmdResponse = recvMsg(controlSocket, 7)

		if cmdResponse[:5] == 'ERROR':  
			print 'Error received. Try again.' 

		elif cmdResponse[:6] == 'LISTEN':
			#print 'Start listen'	
			dataSocket = startListen(dataPort) 

			cmdResponse = recvMsg(controlSocket, 7)

			if cmdResponse[:5] == 'ACK_l': 
				#print 'ACK_l received'
				dirStr = recvMsg(dataSocket, 512) 
				printDirectory(dirStr)
				dataSocket.close()
			
			elif cmdResponse[:5] == 'ACK_g':
				print 'ACK_g received'

				cmdResponse = recvMsg(controlSocket, 7)

				if cmdResponse[:6] == 'FILENF': 
					print 'FILE NOT FOUND' 	
					dataSocket.close()

				elif cmdResponse[:6] == 'FILENG':
					print 'Error: No file given'
					dataSocket.close()
				
				elif cmdResponse[:6] == 'FILEIN': 
					print 'Receiving file'
					fileSize = recvMsg(dataSocket, 256)
					fileSize = fileSize.rstrip('\x00')
					#print 'filesize is ' + int(fileSize)	

					#fileSize = int(fileSize)
					print 'received filesize is ' + fileSize	
					fs = int(fileSize)
					print fs 
					#fileSize = socket.ntohs(fileSize)

					file1 = recvMsg(dataSocket, int(fileSize))
					#filex = file1.rstrip('\x00')

					fileName = cmdRequest[3:]
					f = open(fileName, 'w')
					f.write(file1)
					#with open(fileName, 'a') as f: 
					#	f.write(file1) 	

					f.close()
					print 'file transfer complete'
					#print file1


					dataSocket.close()

'''	
	
		elif cmdResponse[:6] == 'FILENF': 
			print 'FILE NOT FOUND' 	
				
'''
	
	

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


	