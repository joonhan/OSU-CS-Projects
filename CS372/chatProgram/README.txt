//chat program README.txt

1. This program must be run using 2 separate terminal windows. Open two windows and
	navigate to the location of file folder. 

2. To compile chatclient.c using the provided makefile, simply type:
		
		"make"  
	
	while chatclient.c and makefile are in the same directory. Or to compile 
	manually, type: 
		
		"gcc chatclient.c -o chatclient"


3. Start chatserve.py in another window by typing: 

		"python chatserve.py 12000"  

	Or whichever port number is available. 


4. On the other window, start chatclient by typing:
		
		"chatclient	localhost 12000" 
	
	Or whichever server/port number. 


5. Type your name in the chatclient window. 

6. Type a message in the chatclient window.

7. Type a message in the chatserve window.

8. Repeat from Step 6

