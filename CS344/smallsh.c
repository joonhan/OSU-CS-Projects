//Joon Han
//11/7/2016
//Basic shell program

#include <stdio.h>	
#include <stdlib.h>
#include <sys/types.h>  
#include <unistd.h>		
#include <stdbool.h> 
#include <string.h>
#include <signal.h>
#include <fcntl.h>		

 
//this function is used to periodically check for the status of background processes
void checkBgProcesses() {
	pid_t child_pid;
	int child_status; 

	//this sleep is used to delay the checking of the child pid so it is caught by the waitpid() below; without it
	//when manually killing a background process, the waitpid() will run before receiving the signal from the child, thereby
	//requiring an extra iteration of the prompt loop (ie requires you to press "ENTER" again to get the message)
	usleep(1000); 

	//store the child pid of theh process waited for
	child_pid = waitpid(-1, &child_status, WNOHANG);

	//if child pid is given..
	if (child_pid > 0) {

		//if it was exited normally, print relevant message
		if (WIFEXITED(child_status)) {	
			printf("%s%d%s%d\n", "background pid ", child_pid, " is done: exit value ", WEXITSTATUS(child_status)); 
			fflush(stdout);
		}

		//if it was killed by a signal, print relevant message
		else if (WIFSIGNALED(child_status))	{
			printf("%s%d%s%d\n", "background pid ", child_pid, " is done: terminated by signal ", WTERMSIG(child_status)); 
			fflush(stdout);
		}
	}
}


//this function runs the loop for program
void runShell() {
	char* inputString; 				//store user input as string
	size_t inputSize = 0; 			//set size of input string
	ssize_t lineLength = 0;			//store # of chars in input string (ssize_t also holds -1 for errors); 

	char* command; 					//store the first command from user input	
	char* argsArray[512]; 			//array of pointers to argument strings; max 512 arguments
	int argsIndex = 0;				//index to the array of arguments
	char* arg;							//pointer to the current substring argument

	bool isBackground = false;  	//flag to detect if background process
	pid_t bgProcesses[100];			//store background processes in array
	int bgProcessIndex = 0; 		//index for the background array

	int status_child = 0; 			//store the status code when waiting for child

	int inFile; 						//file descriptors for input file and output file
	int outFile; 					

	char* inFileName; 				//pointers to input and output file names
	char* outFileName;

	//create a sigaction struct to deal with signals 
	struct sigaction sa_parent;

	//point the handler so that parent process ignores all interrupt signals 
	sa_parent.sa_handler = SIG_IGN;	

	//when SIGINT occurs it will signal the parent
	sigaction(SIGINT, &sa_parent, NULL);


	//main loop that controls the shell prompt
	while (1) {

		//reset variables for each iteration
		isBackground = false; 		
		argsIndex = 0; 	
		inFileName = NULL;
		outFileName = NULL;

		//periodically check and see if background child processes are complete
		checkBgProcesses(); 

		//print prompt
		printf("%s", ": ");
		fflush(stdout); 

		//get length of input string
		lineLength = getline(&inputString, &inputSize, stdin); 

		//removes the newline when inputString exists (if not removed, it fails the strcmp!)  
		strtok(inputString, "\n");						 

			//INPUT FORMAT 
			// command [arg1 arg2 ...] [< input_file] [> output_file] [&]

		//store the first substring as 'command' (i.e. "ls", "cd", etc.)
		command = strtok(inputString, " ");											//inputString is changed! 

		// ==== BUILT-IN COMMANDS ========================================
		
		//exit shell
		if (strcmp(command, "exit") == 0) {
			//kill background processes

			int i; 
			for (i = 0; i < bgProcessIndex; i++) {
				kill(bgProcesses[i], SIGTERM); 			
			}
			
			exit(0);  
		}

		//change directory
		else if (strcmp(command, "cd") == 0) {	
			
			char* cdPath;  

			//point to the given directory, if given in argument
			cdPath = strtok(NULL, " "); 

			//if no cd path is given, go to path in HOME	
			if (cdPath == 0) {
				const char *myHome = "HOME";
				
				cdPath = getenv(myHome); 
				chdir(cdPath);
		
				//prompt again
				continue; 		
			}

			else {
				chdir(cdPath);
				continue;
			}
		}	

		//check status of last foreground process
		else if (strcmp(command, "status") == 0) {

			//self-note: WIFEXITED returns 1 if child process completes normally (i.e. even with invalid input, the child
			//process still closed properly); it was not terminated using a signal or kill but ended on its own
	
			//prints the status message when child process exits normally
			if (WIFEXITED(status_child)) { 						//returns a non-zero if child terminated normally
				printf("%s%d\n", "exit value ", WEXITSTATUS(status_child)); 	
				fflush(stdout); 
			}
			//print the status message when child process did not exit normally, 
			else if (WIFSIGNALED(status_child)) {				//child process was terminated by a signal
				printf("%s%d\n", "terminated by signal ", WTERMSIG(status_child)); 
				fflush(stdout); 
			}
			continue; 
		}
		// ==== END OF BUILT-IN COMMANDS =====================================


		//check if input is a comment or the ENTER key is pressed (*command returns its ascii value) 
		else if (inputString[0] == '#' || (*command) == 10) {
			continue; 
		}

		//store 'command' as the first element in array and increment
		argsArray[argsIndex] = command;						
		argsIndex++; 												

		//get the 1st argument (argv[1]) from the input string
		arg = strtok(NULL, " ");							

		//while an argument exists, process it 
		while (arg != 0) {

			//check for redirection of input and extract next arg
			if (strcmp(arg, "<") == 0) {
				
				arg = strtok(NULL, " "); 

				if (arg == 0) {
					printf("%s\n", "input file not chosen"); 
					fflush(stdout); 

					//stops the command from running during exec() if invalid argument is given
					argsArray[0] = 0;
				}

				else {
					inFileName = arg;					//a valid inputfile is now pointed to
				}	
			}

			//check for redirection of output
			else if (strcmp(arg, ">") == 0) {
				
				arg = strtok(NULL, " "); 

				if (arg == 0) {
					printf("%s\n", "output file not chosen");
					fflush(stdout); 
					
					//prevent command from running during exec()
					argsArray[0] = 0; 
				}

				//assign the name of output file 
				else {
					outFileName = arg;  
				}
			}

			//if argument has a '&' then turn on background flag
			else if (strcmp(arg, "&") == 0) {
				isBackground = true; 	
			}
		
			//for arguments without redirection or background, store arguments into the array
			else {
					argsArray[argsIndex] = arg;
					argsIndex++;
			}
				
			//get the next argument
			arg = strtok(NULL, " ");				

		} //end of while loop (for args)

		//assign NULL as the last element of the argument array (needed for execvp())
		argsArray[argsIndex] = NULL;			

		pid_t pid = -10; 
		
		//create child process
		pid = fork(); 
	
		int devnull; 

		//child process	
		if (pid == 0) {
			//create signal action so that foreground child processes are affected by default behavior of SIGINT 
			struct sigaction sa_child;
			sa_child.sa_handler = SIG_DFL;					//interrupt signals cause default action to child processes
			sigaction(SIGINT, &sa_child, NULL); 

			//if input file exists
			if (inFileName != NULL) {
				//open the input file; handle errors
				inFile = open(inFileName, O_RDONLY); 	

				if (inFile == -1) {											
					perror("Error opening infile");
					exit(1); 
				}

				//attempt to redirect child's stdin to the inputfile file descriptor (3); 
				if (dup2(inFile, 0) == -1) {								
					perror("Error redirecting infile");					//if successful, the inputfile is now acting
					exit(1);														//as the stdin..
				}
				close(inFile); 
			}
			//if output file exists
			if (outFileName != NULL) {
				outFile = open(outFileName, O_WRONLY|O_CREAT|O_TRUNC, 0644); 	//write only | create if doesn't exit | write
																									//over previous file contents if file exists
				//error opening file
				if (outFile == -1) {
					perror("Error opening outfile");
					exit(1); 
				}
				//error redirecting
				if (dup2(outFile, 1) == -1) {
					perror("Error redirecting outfile");
					exit(1);
				}
				close(outFile); 
			}
		
			//if it's a background process, redirect stdin to dev/null	
			if (isBackground == true) {
				//attempt to open dev/null
				devnull = open ("/dev/null", O_RDONLY);
				if (devnull == -1) {
					perror("Error redirecting input to dev/null");
					exit(1);		
				}
				if (dup2(devnull, 0) == -1) {
					perror("Error redirecting to dev/null");
					exit(1);
				}		
			
				//make background child processes ignore interrupt signals
				sa_child.sa_handler = SIG_IGN;
				sigaction(SIGINT, &sa_child, NULL); 
			}

			//child process executes a new program; if it fails, return error message
			if (execvp(argsArray[0], argsArray) == -1) {
					perror(argsArray[0]);			
					exit(1); 							//if command is invalid, close process 
			}
		}	//end of child process

		//parent process
		else if (pid > 0) {
	
			//if it's a background process	
			if (isBackground == true) { 

				//store child process in array
				bgProcesses[bgProcessIndex] = pid; 
				bgProcessIndex++; 

				//print message after starting background process
				printf("%s%d\n", "background pid is ", pid);
				fflush(stdout); 
			}	
			
			//child process is foreground process	
			else {
				//parent waits for child with specific pid
				waitpid(pid, &status_child, 0);

				//any foreground child process terminated by signal will print message
				if (WIFSIGNALED(status_child)) {
					printf("%s%d\n", "terminated by signal ", WTERMSIG(status_child)); 
					fflush(stdout); 
				}

				/*if (status_child == 2) {
					printf("%s%d\n", "terminated by signal ", status_child); 
				}*/
 			}																			
		}
	
		//error occurs during fork()
		else {					
			perror("ERROR"); 
			exit(1);
		}

	} //end of while loop

} //end of runShell()



int main(int argc, char* argv[]) { 

	runShell();
	
	return 0; 
}




