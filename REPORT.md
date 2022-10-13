# Simple Shell

Explain how you transformed the assignment into code
Not "What does your code do?"
But "How does it do it?"

Focus on how your program works at the time you submit it

Don't go too much into the details of everything you tried to get there

Mention the limitations or peculiarities of your program if any

200 lines maximum
Generally 125-150 lines are perfectly enough!

## Summary

This program will create a shell that will take command-line inputs and execute them correctly. While practicing high-quality C code to implement a shell which deals with parsing command-line arguments accordingly by utilizing error management in order to execute various system calls with an emphasis on processes, files and pipes.

## Implementation

1. Parsing the command line arguments and identifying behavior of each command and storing them in a struct and linking them with a doubly linked list. 
2. Executing commands successfully based on their system calls or recognizing proper misbehavior and reporting the specific error to the terminal
3. Displaying return statuses in a completion message and then continuously prompting the user to enter another command after execution until they want to exit

### Parsing Implementation 

We begin our execution with creating a data structure which will contain the data of our commands. This enables us to identify all of the information that a certain command contains and allow us to access the data at any time. Our `struct aCommand` contains:
	>>**processName**: Which contains the main command to be executed and serves as our first parameter in our `execvp` call
	>>**redirectFileName**: Which contains the filename if a file will need to be manipulated, it also serves as a way for us to identify if output or input redirection is needed
	>>**arguments[ARGS_MAX]**: Which contains an array which stores tokens from the command line with a limitation of 16 arguments, this allows us to identify specific syscalls and manipulate the information given
	>>**cmdSave[CMDLINE_MAX]** Which contains a copy of the original command line for printing out the completion message
	>>**numOfArgs**: The number of arguments allows us to ensure that arguments in specific commands are parsed correctly and to error check the maximum number of arguments
	>>**fd[2] & pid**: The strategy of each command having their own file descriptors and pid is shown in our piping in which we iterate through each command and will need to identify the current command's pid and file descriptor. 


// explain doubly linked list 
Here is a simple footnote [^2]

### Syscall Performance

// Simple commands

// File manipulation 
	// input redirection
	// output redirection
	// manipulation with piping 

// Piping 

### Dealing with Memory 

// talk about malloc and 

### Limitations and Peculiarities 

Our program is limited as noted in the Project1.html document to only work with a maximum of sixteen arguments. In order to limit this, we can use our struct member `numOfArgs` to check if there are more than 16 arguments given. 
