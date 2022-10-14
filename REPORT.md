# Simple Shell

## Summary

This program will create a shell that will take command-line inputs and execute
them correctly. While practicing high-quality C code to implement a shell which
deals with parsing command-line arguments accordingly by utilizing error
management in order to execute various system calls with an emphasis on
processes, files and pipes.

## Implementation

1. Parsing the command line arguments and identifying behavior of each command
and storing them in a struct and linking them with a doubly linked list. 
2. Executing commands successfully based on their system calls or recognizing
proper misbehavior and reporting the specific error to the terminal  
3. Displaying return statuses in a completion message and then continuously
prompting the user to enter another command after execution until they want to
exit

### Parsing Implementation

We begin our execution with creating a data structure which will contain the
data of our commands. This enables us to identify all of the information that a
certain command contains and allow us to access the data at any time. We iterate
through each token from the command line until it is empty, and will be able to
identify whether there is a specific process such as file manipulation or
piping. Our `struct aCommand` contains: <br> 
>>**processName**: The main command
to be executed and serves as our first parameter in our `execvp` call <br>
>>**redirectFileName**: The filename if a file will need to be manipulated, it
also serves as a way for us to identify if output or input redirection is needed
<br>
>>**arguments[ARGS_MAX]**: An array which stores tokens from the command
line with a limitation of 16 arguments, this allows us to identify specific
syscalls and manipulate the information given <br>  >>**cmdSave[CMDLINE_MAX]** A
copy of the original command line for printing out the completion message with a
maximum of 512 <br>
>>**numOfArgs**: The number of arguments allows us to
ensure that arguments in specific commands are parsed correctly and to error
check the maximum number of arguments <br>
>>**fd[2] & pid**: The strategy of
each command having their own file descriptors and pid is shown in our piping in
which we iterate through each command and will need to identify the current
command's pid and file descriptor. <br>
<br>
Secondly, if a pipe is identified, we then store each of our `aCommand` objects
into a doubly-linked list. This is done after parsing and storing our commands
and they are then assigned to two pointers `p and p_tail` which represents the
current item of the list and the last item of the list respectively. We then
keep track of the items in our list through the choice of a doubly-linked list.
The advantage of this is that we are able to manipulate and check the previous
and next command of the current command. This is especially useful when
identifying a command that will need to access the `STD_OUT` of the previous
command and provide the `STD_IN` to the next command. **_Giving credit to Joël
Porquet-Lupine Project 0: sgrep.c from ECS150 Fall 2022 in which was then
modified in our program to be a doubly linked list_**

### Syscall Performance

We have separated our execution into four processes to ensure that the command
will behave correctly based on the system calls they are given.

#### Simple commands: Create Process

With simple commands, our program will identify any built in commands, add it to
our command struct and execute them accordingly.

#### File Manipulation

With output and input redirection, we identify if the command contains a `>` or
`<` character and stores the next argument into our `redirectFileName` in our
struct. We then will be able to open the file if it is valid and then save the
input or output depending on the command.

##### File manipulation with piping

In order to ensure that file manipulation is possible when piping, output and
input redirection is identified in the child process and will then open the
file. This is following the limitation that there is only supposed to be input
manipulation in the first command and ouput manipulation in the last command.

#### Create Process with Piping

Our piping is efficient in which it utilizes our doubly linked list to identify
the file descriptors of our previous and next command. We iterate through our
number of commands in a for loop and identify if it is the first command, the
middle command, or the last command. We then execute the commands and once they
have all ran, we then wait for the status values and print them to the terminal.

### Limitations and Peculiarities

Our program is limited as noted in the Project1.html document to only work with
a maximum of sixteen arguments. In order to limit this, we can use our struct
member `numOfArgs` to check if there are more than 16 arguments given. We
recognize the peculiarities of the repetition and inefficiency in condensing our
code, but as full time college students running on coffee we are prepared to
face the sacrifices in order to submit a fully functional program.
