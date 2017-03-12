//Joon Han Project 2 README.txt

Summary: Compile ftserver.c and run in one window. Run ftclient.py in a separate window and input commands.

Step-by-Step Instructions

1. To compile ftserver.c using the provided makefile, simply type:
		
		"make"  
	
	while ftserver.c and makefile are in the same directory. Or to compile 
	manually, type: 
		
		"gcc ftserver.c -o ftserver"


2. Run ftserver by typing "ftserver 30030" or any valid port number. The terminal will read: 
		
		"Server is listening on port 30030"


3. Run ftclient.py in another window by typing: 

		"python ftclient.py flip 30030 30040"  

	Or whichever port number is available. 


4. A prompt with ">" will appear in the window. 
		

5. Type '-l' to view the server's directory. The directory will be printed on screen and the ">" prompt should 
	reappear. *Note: the control connection is persistent in this program. 

6. Type '-g [filename]' to transfer files to the client's directory. 'filename' is a file in the server's directory. 
	The terminal should show a 'transfer is complete' message. 

	Note: any duplicate file names will have a number appended to the end as follows: "filename_1_.txt" 
	Note: If no filename is provided or an invalid filename is given, the program will display an error message. 


7. Type in "quit" or ctrl+c to exit the client program. The server program will start listening again for new clients. 


8. Press ctrl+c in the server's window to stop the server program. 

Extra Credit:
 * Type '-cd', '-cd [path]', or '-cd ..' to move through the directory. 



