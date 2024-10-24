/** Assignment: Project 4 - A Simple Shell
 * 	File Name: simple-shell.c
 * 	Author: McEwan Bain
 * 	Desc:This project consists of designing a C program to serve as a shell interface that accepts user commands and 
 * 		 then executes each command in a separate process
 *	Notes: :P
*/

#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <stdbool.h>


#define MAX_LINE		80 /* 80 chars per line, per command, should be enough. */
#define MAX_COMMANDS	9 /* size of history */

char history[MAX_COMMANDS][MAX_LINE]; //the array used to store history commands.
char display_history [MAX_COMMANDS][MAX_LINE]; //the array used for "printf" to display history nicely

int command_count = 0;

/**
 * Add the most recent command to the history.
 */

void addtohistory(char inputBuffer[]) {
	//Local variables
	int i = 0; //Loop control var

	//Adding command to history
	strcpy(history[command_count], inputBuffer);

	// update array"display_history": remove characters like '\n', '\0' in order to display nicely
	while(inputBuffer[i] != '\n') {
		if(inputBuffer[i] == '0') {
			display_history[command_count][i] = ' ';
		} else {
			display_history[command_count][i] = inputBuffer[i];
		}
		i++;
	}
	display_history[command_count][i] = '\0'; //adding string terminator to end of display history
	
	//Updating command count
	command_count++;
	if(command_count >= MAX_COMMANDS) {
		command_count = 0;
	}
	return;
}

/** 
 * The setup function below will not return any value, but it will just: read
 * in the next command line; separate it into distinct arguments (using blanks as
 * delimiters), and set the args array entries to point to the beginning of what
 * will become null-terminated, C-style strings. 
 */

int setup(char inputBuffer[], char *args[],int *background) {
    int length,		/* # of characters in the command line */
	i,				/* loop index for accessing inputBuffer array */
	command_number;	/* index of requested command number */

	//define your local varialbes here;
	int currArgNum = 0;
	bool newArg = true;
	int historyNum = 0; //The number following a '!' when the user wishes to use a specific previous command
	int index = 0; //Used to store current history index
	
    /* read what the user enters on the command line */
	do {
		printf("osh>");
		fflush(stdout);
		length = read(STDIN_FILENO,inputBuffer,MAX_LINE); 
	}
	while (inputBuffer[0] == '\n'); /* swallow newline characters */


    if (length == 0)
        exit(0);            /* ^d was entered, end of user command stream */
 
    if ( (length < 0) ) {
		perror("error reading the command");
		exit(-1);           /* terminate with error code of -1 */
    }
	
	/**
	 * Check if they are using history
	 */
	if(inputBuffer[0] == '!' ) {
		if(inputBuffer[1] == '!') { // Use most recent history command
			index = command_count - 1;
			
			//Checking that there is a command stored
			if(strlen(history[command_count-1]) > 0) {
				strcpy(inputBuffer, history[command_count-1]);
			} else {
				printf("No commands in history\n");
				return 0;
			}
		} else { 
			historyNum = inputBuffer[1] - '0'; //use nth command in history
			index = (command_count - historyNum - 1 + MAX_COMMANDS) % MAX_COMMANDS;
			//Checking that there is a command stored
			if(strlen(history[index]) > 0) {
				strcpy(inputBuffer, history[index]);
			} else {
				printf("No such command in history\n");
				return 0;
			}
		}
		length = strlen(inputBuffer);
	}
	
	

	/**
	 * Add the command to the history
	 */
	addtohistory(inputBuffer); 
	
	/**
	 * Parse the contents of inputBuffer
	 */
	
    for (i=0;i<length;i++) { 
		/* examine every character in the inputBuffer */
		
        switch (inputBuffer[i]){
			/* argument separators */
			case ' ': //When curr char is a space(s), next char is the start of an arg, curr char should be \0
			case '\t' : //When curr char is a tab(s), next char is the start of an arg, curr char should be \0			
				inputBuffer[i] = '\0';
				newArg = true;
				break;

			case '\n':  /* should be the final char examined */
				// set up the last item args[x] == NULL;
				inputBuffer[i] = '\0';
				args[currArgNum] = NULL;
				break;

			//I found it easier to add a switch case for & rather than deal w/ it in default
			case '&': //'&' Handling, if char is an ampersand
				if(newArg == true && inputBuffer[i+1] == '\n') { //& must be by itself and be the last args
					*background = 1;
					//printf("Set\n");
				}
				break;

				
	    	default :             /* some other character */
				//If this is the first char or the prev char was whitespace, this char is the first char of next arg
				if(newArg == true) {
					
					args[currArgNum] = &inputBuffer[i];
					currArgNum++;
				}
				newArg = false;
				break;
		} /* end of switch */
	}    /* end of for */
	return 1;
	
} /* end of setup routine */


int main(void) {
	char inputBuffer[MAX_LINE]; 	/* buffer to hold the command entered */
	int background;             	/* equals 1 if a command is followed by '&' */
	char *args[MAX_LINE/2 + 1];	/* command line (of 80) has max of 40 arguments */
	pid_t child;            		/* process id of the child process */
	
	//define your local variables here, at the beginning of your program. 
	int i, j; //Loop control var
	int execStatus = 0; //Holds return value of execvp
	int shouldrun = 1;
	int* status = 0; //Wait status handler
	bool historyFlag = false; //Used to skip exec and fork loop when history is entered
	

		
    while (shouldrun) {            		/* Program terminates normally inside setup */
		historyFlag = false;
		background = 0;

		shouldrun = setup(inputBuffer, args, &background);       /* get next command */
		if(shouldrun == 0) {
			return 0;
		}

		//If agrs[0] == NULL, terminates to avoid issues. Should only really be an issue if user enters "&" by itself
		if(args[0] == NULL) {
			printf("Terminated abnormally: agrs[0] == null\n");
			return 0;
		}

		//Exit Handling
		if(strncmp("exit", args[0], 4) == 0 && args[1] == NULL) { //Returns 0 if user enters "exit"
			return 0; //Quit program
		}
		
		// if the user typed in "history", the shell program will display the history commands. Displays 8 commands (9-history)
		if(strncmp("history",args[0], 7) == 0 && args[1] == NULL) {
			printf("Command History\n----------------\n");

			i = 1; //Starting from the command after history
			while (i < MAX_COMMANDS) {
    			// Calculate the index of the command to print
    			j = (command_count - 1 - i + MAX_COMMANDS) % MAX_COMMANDS;  // handle wrap-around

    			// Check if there's a valid command in the display_history array
    			if (strlen(display_history[j]) > 0) {  // only print if not empty
    	    		printf("%d: %s\n", i, display_history[j]);  // show history number starting from 1
				}	
				i++;
			}
			historyFlag = true; //Skips next section of the function and returns to the top of while loop
		}
		
	
		
		if (shouldrun && historyFlag == false) {
			/* creates a duplicate process! */
			child = fork();
			if(child == 0) { //Child process, calls execvp
				execStatus = execvp(args[0], args); //System call to run (command, params)
				
				//Error Handling
				if(execStatus == -1) {
					printf("Invalid Command and/or Parameters\n");
				}
				exit(-1);
				
			} else if(child > 0) { //Parent process,
				if(background == 0) { //If no background flag was set, parent must wait for child
					waitpid(child, status, WCONTINUED);
				} else {	/*else if(background == 1){}*/ //If background flag was set, child may run concurrently
					//printf("Background Succesfull\n");
				}
			} else if(child < 0) { //Pid < 0, error occured
				printf("Fork Error Occurred, terminating program\n");
				return 0;
			}
		}
    }
	
	return 0;
}

