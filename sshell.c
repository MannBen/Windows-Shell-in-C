#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>

#define CMDLINE_MAX 512
#define ARGS_MAX 16

static inline void *xmalloc(size_t size){ //professors provided project 0 implementation of linked list structs
    void *p = malloc(size);
    return p;
};
//print command exit status to stderr
void fprintCommandStatus(char cmd[CMDLINE_MAX], int status){
    fprintf(stderr, "+ completed '%s' [%d]\n",
                cmd, WEXITSTATUS(status));  //builtin stderr print with wexitstatus
}
//print builtin and simple command int value to stderr instead of status
void fprintCommand(char cmd[CMDLINE_MAX], int status){
    fprintf(stderr, "+ completed '%s' [%d]\n",
                cmd, status);
}

// Giving credit to Joël Porquet-Lupine Project 0: sgrep.c from ECS150 Fall 2022 
// in which was then modified in our program to be a doubly linked list_
//linked list used to house 2-4 commands used for piping, currenly hardcoded for 2-4 but scaleability possible
struct commandList{
    struct commandListNode *p_head, *p_tail; //head holds pointer to top (access) link in linked list / tail holds pointer to last value
    int numOfCommands; //num of commands for a given commandList (how many nodes in the linked list)
};
struct commandListNode{
    struct aCommand *command; //the data we care about our linked list holding
    struct commandListNode *next, *prev; //next is pointer to next node in linked list null for tail / prev is pointer to previous node in linked list null for head
};

struct aCommand{  //instace of this stuct is a fully prepared command to be used to exec
    char *processName; //arg[0] / first command given to execvp
    char *redirectFileName; //if true command will pipe either in or output somewhere
    char *arguments[ARGS_MAX]; //max size 16, contains arguments for a given process
    char cmdSave[CMDLINE_MAX]; //saves full user inputted stdin from terminal
    int numOfArgs; //number of arguments added to *arguments
    int fd[2]; //file descriptors used for piping in createProcessWithPipes
    pid_t pid; //saves the pid of the child that runs execvp
};
//pushd/popd/dir stack struct/struct funcs
//referenced https://stackoverflow.com/questions/1919975/creating-a-stack-of-strings-in-c for structure of a single link stack 
struct stack{ //stack
  struct stackNode *headOfStack; //top of the stack
  int stackSize; //number of objects in the stack
};
struct stackNode{ //stack entyr
  char *dir; //data we care about 
  struct stackNode *next; //pointer to nect node in the list
};
struct stack *createNew(void){ //initialize link list stack
  struct stack *stackRet = xmalloc(sizeof(*stackRet)); //malloc with no free <-fix
  if (stackRet){ //if successful malloc initialize stack vars
    stackRet->headOfStack = NULL;
    stackRet->stackSize = 0;
  }
  return stackRet; //return struct object to caller =
};
//helper stack funcs
void push(struct stack *currStack, char* dirToPush){ //push a dirtToPush onto the stack currStack
    struct stackNode * stackToEnter = xmalloc(sizeof *stackToEnter); //freed when exit is called
    char * copy = malloc(strlen(dirToPush) + 1); //pointer to new string initalized by malloc freed by exit/pop()
    if (stackToEnter){ //if it exists
        strcpy(copy, dirToPush); 
        stackToEnter->dir = copy; //data we care about set to top of the stack
        stackToEnter->next = currStack->headOfStack; //next entry of the stack is set to current head / builds down 
        currStack->headOfStack = stackToEnter; //new head of the stack is dirToPush
        currStack->stackSize++; //increase stack size
    }
}
void pop(struct stack *currStack){
    if (currStack->headOfStack != NULL && ((currStack->stackSize-1) != 0)){ //if the stack is not size 1 currently (unless exit is called) and stack->head exists 
        chdir(currStack->headOfStack->next->dir); //return to stack-1 dir
        free(currStack->headOfStack->dir); //free top of stack
        currStack->headOfStack = currStack->headOfStack->next; //slide head to new dir set above
        currStack->stackSize--; //decrease size of stack
        fprintf(stderr, "+ completed '%s' [%d]\n",  //stderr success completed
                "popd", 0);
    }
    else{
        fprintf(stderr, "Error: directory stack empty\n"); //sterr failure
        fprintf(stderr, "+ completed '%s' [%d]\n",
                "popd", 1); //completed
        return;
    }
}

int getNumPipeCommands(char cmd[ARGS_MAX]){ // returns the number of pipes our current user in is calling for
    int numCommands = 1; //return value starts at one as if we have 1 pipe char we have min 2 pipes
    char cmdCopy[CMDLINE_MAX];  //to be butchered below as to perserve cmd intergrety 
    char* n1;
    strncpy(cmdCopy, cmd, CMDLINE_MAX); //make copy <- potential mem leak
    while (1){
        n1 = strchr(cmdCopy, '|'); // if pipe char exists in the string
        if (n1){ //set it to empty space char
            *n1 = ' '; 
            numCommands++; //return value 
        }
        else{ //no more pipe chars found
            break; 
        }
    }
    return numCommands;
}

void createProcess(struct aCommand *currCommand){//simple create proccess
    pid_t pid;
    pid = fork(); //fork to make child process that will call execvp to preserve our sshell
    int status = -1;
        if (pid == 0) {//while in the child process
            // child
            execvp(currCommand->processName, currCommand->arguments);  //execvp with processName, arguments
            //perror("execvp");
            fprintf(stderr,"Error: command not found\n");
            exit(1);
        } 
        else if (pid > 0) {//the parent process
            waitpid(pid, &status, 0); //wait for return of child process
        }
        else {
            perror("fork");
            exit(1);
        }
        // show status 

        fprintCommandStatus(currCommand->cmdSave, status); //print out completion message and return status w/ WEXITSTATUS
}
void createProcessWithOutputRedirection(struct aCommand *currCommand){//simple create process with known outputredir required
    pid_t pid;
    pid = fork(); //fork to make child process that will call execvp to preserve our sshell
    int status = -1, toFileFD = 0, savedSTDO = 0;
    if (pid == 0) {//the child process
        if(currCommand->redirectFileName == NULL){ //catch no output file
            fprintf(stderr, "Error: no output file\n");
            return;
        }
        if((toFileFD = open(currCommand->redirectFileName, O_WRONLY | O_CREAT, 0644)) == -1 || strchr(currCommand->redirectFileName, '/')) { //if open returns > 0 and the file name is not '/'
            fprintf(stderr, "Error: cannot open output file\n");
            return;
        }
        savedSTDO = dup(STDOUT_FILENO); // save stdout
        dup2(toFileFD, STDOUT_FILENO); //replace stdout
        close(toFileFD); //close stdout, buffer still active
        execvp(currCommand->processName, currCommand->arguments);  //execvp with processName, arguments
        perror("execvp");
        exit(1);
    } 
    else if (pid > 0) {//the parent process
        waitpid(pid, &status, 0); // wait for child to complete
        fflush(stdout); //flush buffer (not required)
        dup2(savedSTDO, toFileFD); //return to STDO (main will do this too)
        close(savedSTDO); //close stdout
    } 
    else {
        perror("fork");
        exit(1);
    }
    fprintCommandStatus(currCommand->cmdSave, status); //print out completion message and return status w/ WEXITSTATUS
}
void createProcessWithInputRedirection(struct aCommand *currCommand){//simple create process with known inputredir required
    pid_t pid;
    pid = fork();//fork to make child process that will call execvp to preserve our sshell
    int status = -1, toFileFD = 0, savedSTDO = 0;
    if (pid == 0) {
        // child
        if(currCommand->redirectFileName == NULL){ //catch no output file
            fprintf(stderr, "Error: no input file\n");
            return;
        }
        else if((toFileFD = open(currCommand->redirectFileName, O_RDONLY, 0644)) == -1 || strchr(currCommand->redirectFileName, '/')){ // no file/file dne
            fprintf(stderr, "Error: cannot open input file\n");
            return;
        }
        savedSTDO = dup(STDIN_FILENO); //save stdout
        dup2(toFileFD, STDIN_FILENO); //replace stdout
        close(toFileFD);
        execvp(currCommand->processName, currCommand->arguments);  //execvp with processName, arguments
        perror("execvp");
        exit(1);
    } 
    else if (pid > 0) {
        // parent
        waitpid(pid, &status, 0); //wait for child to finish
        fflush(stdout);
        dup2(savedSTDO, toFileFD);//return to STDO
        close(savedSTDO);
    } 
    else {
        perror("fork");
        exit(1);
    }
    fprintCommandStatus(currCommand->cmdSave, status); //print out completion message and return status w/ WEXITSTATUS
}

void createProcessWithPipes(struct commandList *cmdList){
    struct commandListNode *access = xmalloc(sizeof(struct commandListNode)); //initialize access commandListNode
    access = cmdList->p_head; //set access to point to head of linkList
    int status;
    for (int i = 0; i < cmdList->numOfCommands; i++) {
        if(i != cmdList->numOfCommands - 1) { //for n commands make n-1 pipes 
            pipe(access->command->fd); //pipe current access fd[]
        }
        access->command->pid = fork(); //fork set to current access pid value
        if(access->command->pid == 0) { // child
            if(i == 0) { // first command
                if (access->command->redirectFileName != NULL){ //check for input redirection as can only happen in first command
                    int toFileFD = open(access->command->redirectFileName, O_RDONLY, 0644);  //open file for input redir
                    dup2(toFileFD, STDIN_FILENO); //replace stdout
                    close(toFileFD);
                }
                close(access->command->fd[0]); //close stdin
                dup2(access->command->fd[1], STDOUT_FILENO); //replace stdout (1) with current access fd[1]
                close(access->command->fd[1]); //close current access fd[1]
            }
            else if (i == cmdList->numOfCommands - 1 ) { // last command 
                close(access->prev->command->fd[1]); //close write of prev access (command before current command)
                dup2(access->prev->command->fd[0], STDIN_FILENO); //open and replace stdin (0) with current fd[0]
                close(access->prev->command->fd[0]); //close prev access (fd[0]) 

                if (access->command->redirectFileName != NULL){ //check for outputredirection as can only happen in last command
                    int toFileFD = open(access->command->redirectFileName, O_WRONLY | O_CREAT, 0644); //open file for output redir
                    dup2(toFileFD, STDOUT_FILENO); //replace stdout
                    close(toFileFD);
                }
            } 
            else {    // middle command can be 2nd or 3d command / know there is no in or output redir if it reaches this point
                close(access->command->fd[0]); //close read in for curr access fd[0]
                dup2(access->command->fd[1], STDOUT_FILENO); //replaces stdout with current access fd[1]
                close(access->command->fd[1]); //closes current access fd[1]

                close(access->prev->command->fd[1]); //close write for prev access fd[1]
                dup2(access->prev->command->fd[0], STDIN_FILENO); //replace stdin with prev access fd[0]
                close(access->prev->command->fd[0]); //closes prev access fd[0]
            }
            execvp(access->command->processName, access->command->arguments); //exec the current command
        }
        //child ends
        if (i != 0) {   //not equal to A then reference previous file descriptors and close them
            close(access->prev->command->fd[0]); //close prev 0 from parrent
            close(access->prev->command->fd[1]); //close prev 1 from parrent
        }
        if(access->next != NULL) { //if we're not at end of linked list, itterate to next access in list
            access = access->next; //next access
        }
    }
    access = cmdList->p_head; //restore access to begining of linked list for waitpid itter

    int* statusArr = malloc(cmdList->numOfCommands); //freed at end of function

    for (int n = 0; n < cmdList->numOfCommands; n++) {//for number of nodes in linked list / for number of commands
        waitpid(access->command->pid, &status, 0); //wait pid of current access, waits for all children with execvp calls
        statusArr[n] = WEXITSTATUS(status); //store wexitstatus(status) as an int in int array
        if(access->next != NULL) {
            access = access->next; //itterate to next if not at end of linked list
        }
    }
    access = cmdList->p_head; //restore access to begining of linked list for waitpid itter
    fprintf(stderr, "+ completed '%s' ", access->command->cmdSave); //completion print
    
    // print out the number of commands 
    int statusValue = 0;

    while(statusValue < cmdList->numOfCommands) { //status of completed process prints
        fprintf(stderr, "[%d]", statusArr[statusValue]);
        if (statusValue == cmdList->numOfCommands - 1) {
            fprintf(stderr, "\n"); //end of print
        }
        statusValue++;
    }
    free(statusArr); //free temp statusArr holding status values as ints
    free(access); //free access commandListNode that was used to navigate linked list
}
    
void parseInPipes(struct commandList cmdList, char cmd[CMDLINE_MAX]){

    //create and initialize currCommand objects to be put into linked list(hardcoded for 2-4) 
    struct aCommand currCommandOne; 
    memset(&currCommandOne, 0, sizeof(struct aCommand));
    struct aCommand currCommandTwo;
    memset(&currCommandTwo, 0, sizeof(struct aCommand));
    struct aCommand currCommandThree; 
    memset(&currCommandThree, 0, sizeof(struct aCommand));
    struct aCommand currCommandFour;
    memset(&currCommandFour, 0, sizeof(struct aCommand));

    int argumentPos = 1, cmdItr = 1, newCMD = 0;
    char *replace; //used to overwrite < > | 
    strncpy(currCommandOne.cmdSave, cmd, CMDLINE_MAX); //saves initial untokenized cmdIn
    char * token = strtok(cmd, " ");// take initial token 
    currCommandOne.arguments[0] = token; //set initial arg0
    if (strchr(currCommandOne.arguments[0], '|')){ //invalid command 
            fprintf(stderr,"Error: Missing command\n");
            return;
    }
    currCommandOne.processName = currCommandOne.arguments[0]; // set process name for command 1
    while (token != NULL){ //while token -> strtok -> cmd is not null deliminated by " "
        token = strtok(NULL, " "); //get new token after initial is taken 
        if (newCMD && token == NULL){ //error handeling for ls | 
            fprintf(stderr,"Error: Missing command\n");
            return;
        }
        if (token != NULL && (!strchr(token, '|'))){ //token is confirmed not null and is not the pipe character meaning put in args
            if(cmdItr == 1){//currently reading in command 1
                if(strchr(token, '>')) { //can't output redirect in first command / only possible in 2, 3, 4
                    fprintf(stderr,"Error: mislocated output redirection\n");
                    return;
                }
                else if((replace = strchr(token, '<'))){ //check for input redirection
                    *replace = ' '; //replace < with empty char
                    if(strlen(token) > 1) { //handle no spaces between command<file
                        currCommandOne.arguments[argumentPos] = strtok_r(token, " ", &token); //use strtok_r so strtok keeps its place in cmd
                        currCommandOne.redirectFileName = strtok_r(token, " ", &token); //use strtok_r so strtok keeps its place in cmd
                    }
                    else{ //spaces between arg < arg
                        token = strtok(NULL, " "); //get new token that has file
                        currCommandOne.redirectFileName = token; 
                    }
                }
                else{
                    currCommandOne.arguments[argumentPos] = token; //if not < set commandOne.arguments[argumentPos] to token
                }
                argumentPos++; //inc argumentPos
                currCommandOne.numOfArgs = argumentPos;  //update numOfArgs 
            }
            else if(cmdItr == 2){//currently reading in command 2
                 if(strchr(token, '<')){ //cant do input redirection outside of command 1 / not possible in 2,3,4
                    fprintf(stderr,"Error: mislocated input redirection\n");
                    return;
                } 
                else if((replace = strchr(token, '>'))){//check for input redirection
                    *replace = ' '; //replace > with empty char
                    if(strlen(token) > 1) { //handle no spaces between command<file
                        currCommandTwo.arguments[argumentPos] = strtok_r(token, " ", &token); //use strtok_r so strtok keeps its place in cmd
                        currCommandTwo.redirectFileName = strtok_r(token, " ", &token); //use strtok_r so strtok keeps its place in cmd
                    }
                    else{
                        token = strtok(NULL, " "); //get file
                        currCommandTwo.redirectFileName = token; 
                    }
                }
                else{
                    currCommandTwo.arguments[argumentPos] = token;
                }
                argumentPos++;
                currCommandTwo.numOfArgs = argumentPos;
            }
            else if(cmdItr == 3){
                if(strchr(token, '<')){
                    fprintf(stderr,"Error: mislocated input redirection\n");
                    return;
                }
                else if((replace = strchr(token, '>'))){
                    *replace = ' ';
                    if(strlen(token) > 1){ //handle no spaces between command<file
                        printf("\ttokens: %s\t%d\n",token, argumentPos);
                        currCommandThree.arguments[argumentPos] = strtok_r(token, " ", &token);
                        currCommandThree.redirectFileName = strtok_r(token, " ", &token);
                    }
                    else{
                        token = strtok(NULL, " "); //get file
                        currCommandThree.redirectFileName = token;
                    }
                }
                else{
                    currCommandThree.arguments[argumentPos] = token;
                }
                argumentPos++;
                currCommandThree.numOfArgs = argumentPos;
            }
            else if (cmdItr == 4){
                if(strchr(token, '>')){
                    fprintf(stderr,"Error: mislocated input redirection\n");
                    return;
                } 
                else if((replace = strchr(token, '>'))){
                    *replace = ' ';
                    if(strlen(token) > 1){ //handle no spaces between command<file
                        printf("\ttokens: %s\t%d\n",token, argumentPos);
                        currCommandFour.arguments[argumentPos] = strtok_r(token, " ", &token);
                        currCommandFour.redirectFileName = strtok_r(token, " ", &token);
                    }
                    else{
                        token = strtok(NULL, " "); //get file
                        currCommandFour.redirectFileName = token;
                    }
                }  
                else{
                     currCommandFour.arguments[argumentPos] = token;
                }
                argumentPos++;
                currCommandFour.numOfArgs = argumentPos;
            }
            newCMD = 0;
        }
        else{//move and prep for next command            
            currCommandOne.arguments[argumentPos+2] = NULL;//set last arg as null for execvp requirments
            argumentPos = 0;//reset argpost for next command
            cmdItr++;//move to next command
            newCMD = 1;
        }
    }

    currCommandFour.processName = currCommandFour.arguments[0]; //initialize program processNames
    currCommandThree.processName = currCommandThree.arguments[0];
    currCommandTwo.processName = currCommandTwo.arguments[0];

    for (int i = 0; i < cmdList.numOfCommands; i++){
        struct commandListNode *newNode = xmalloc(sizeof(struct commandListNode));
        if (i == 0){
            newNode->command = &currCommandOne;
        }
        else if (i == 1){
            newNode->command = &currCommandTwo;
            if (i != cmdList.numOfCommands-1 && currCommandTwo.redirectFileName != NULL){
                fprintf(stderr,"Error: mislocated output redirection\n");
                return;
            }
        }
        else if (i == 2){
            newNode->command = &currCommandThree;
            if (i != cmdList.numOfCommands-1 && currCommandThree.redirectFileName != NULL){
                fprintf(stderr,"Error: mislocated output redirection\n");
                return;
            }
        }
        else if (i == 3){
            newNode->command = &currCommandFour;
        }
        newNode->next = NULL;
        newNode->prev = NULL;
        if (!cmdList.p_head){
            cmdList.p_head = newNode;
            cmdList.p_tail = newNode;
        }
        else{
            newNode->prev = cmdList.p_tail;
            cmdList.p_tail->next = newNode;
            cmdList.p_tail = newNode;
        }
    }
    createProcessWithPipes(&cmdList);
}
void parseIn(struct aCommand *currCommand, char cmd[CMDLINE_MAX]){
    strncpy(currCommand->cmdSave, cmd, CMDLINE_MAX); //saves initial untokenized cmdIn
    char * token = strtok(cmd, " "); //parse initial token
    currCommand->arguments[0] = token; //set up initial arg0
    currCommand->processName = token; //processName = arg0
    int argumentPos = 1;
    char *replace;
    while (token != NULL){ // while we still have tokens left in cmd, cont
        token = strtok(NULL, " ");
        if (token != NULL){
            // add token to currCommand arguments @mu
            currCommand->arguments[argumentPos] = token;
            if((replace = strchr(token, '>'))){
                *replace = ' '; 
                if (strlen(token) > 1){ // no spaces after and before >
                    token = strtok(token, " ");
                    currCommand->arguments[argumentPos++] = token;
                }
                token = strtok(NULL, " ");
                currCommand->redirectFileName = token;
                 // take new token which will have fileout
                break;
            }
            else if((replace = strchr(token, '<'))){
                *replace = ' '; 
                if (strlen(token) > 1){ // no spaces after and before >
                    token = strtok(token, " ");
                    currCommand->arguments[argumentPos++] = token;
                }
                    token = strtok(NULL, " ");
                    currCommand->redirectFileName = token;
                 // take new token which will have fileout
                break;
            }
            argumentPos++;
        }   
    }
    currCommand->arguments[argumentPos++] = NULL;
    currCommand->numOfArgs = argumentPos; //final number of args
}

// This function will create commands and add them 
int createCommand(char cmd[], struct stack *cdStack){
    char workdir[CMDLINE_MAX];// Variables used in creating cwd stack
    getcwd(workdir,CMDLINE_MAX); // Get the current working directory
    struct commandList cmdList; // Initialize the list of commands 
    memset(&cmdList, 0, sizeof(struct commandList)); 
    cmdList.numOfCommands = 0; // Initialize the number of commands to start at zero because they have not been parsed yet 

    struct aCommand currCommand; // This will be command one
    memset(&currCommand, 0, sizeof(struct aCommand));

    /* Check if the command line contains either | for piping
    > for output redirection or < for input redirection. If not
    then execute command */ 
    if(strchr(cmd, '|')){ // This will mean a pipe is detected
        cmdList.numOfCommands = getNumPipeCommands(cmd); // Check the number of commands and assign to numOfCommand member in struct
        parseInPipes(cmdList, cmd); // Function will then parse the CMD line properly with piping 
    }
    else if(strchr(cmd, '>')){ // This will mean that an output redirection is detected 
        parseIn(&currCommand, cmd); // Parse correctly for output redirection
        // Error catch in where the > is the first argument
        if (strchr(currCommand.arguments[0], '>')){
            fprintf(stderr,"Error: Missing command\n");
            return 1;
        }
        createProcessWithOutputRedirection(&currCommand); // Run process that will implement output redirection  
        return 0;
    }
    else if(strchr(cmd, '<')){ // This means that input redirection is detected
        parseIn(&currCommand, cmd); // Parse correctly for input redirection 
        // Error catch in which < is the first argument 
        if(strchr(currCommand.arguments[0], '<')){
            fprintf(stderr,"Error: Missing command\n");
            return 1;
        }
        createProcessWithInputRedirection(&currCommand); // Run process that will implement input redirection 
        return 0;
    }
    else{
        parseIn(&currCommand, cmd); // parse the output correctly 
        // Error catch in which there are more than 16 process arguments
        if (currCommand.numOfArgs > 15){
            fprintf(stderr,"Error: too many process arguments\n");
        return 0;
        }

        /* Builtin Commands */
        else if (!strcmp(currCommand.arguments[0], "pushd")){ // Command for pushd
            if (currCommand.arguments[1] != NULL) {
                if (chdir(currCommand.arguments[1]) != 0){
                    fprintf(stderr,"Error: no such directory\n");
                    return 1;
                }
            }
            getcwd(workdir,CMDLINE_MAX); // Get the working directory
            push(cdStack, workdir); // Push the working directory to the stack 
            fprintCommand(currCommand.cmdSave, 0); // print out completed message 
        } 
        else if (!strcmp(currCommand.arguments[0], "dirs")){
            struct stackNode * stackToEnter =  cdStack->headOfStack;
            for (int i = 0; i < cdStack->stackSize; i++){
                printf("%s\n",stackToEnter->dir);
                stackToEnter = stackToEnter->next;
            }
            free(stackToEnter);
            fprintCommand(currCommand.cmdSave, 0);
        } 
        else if (!strcmp(currCommand.arguments[0], "popd")){ 
            pop(cdStack);
        } 
        else if (!strcmp(currCommand.arguments[0], "cd")) { // comamnd for cd
            // Error check if the directory is not able to be found 
            if(chdir(currCommand.arguments[1]) != 0) {
                fprintf(stderr,"Error: cannot cd into directory\n");
                fprintCommand(currCommand.cmdSave, 1); // print out completed message with error 1
                return 1;
            }
            fprintCommand(currCommand.cmdSave, 0); // print out completed message 
        }
        else if (!strcmp(currCommand.arguments[0], "pwd")){ // command for pwd 
            printf("%s\n", workdir); // print the working directory for pwd 
            fprintCommand(currCommand.cmdSave, 0); // print out completed message 
        } 
        else if (!strcmp(currCommand.arguments[0], "exit")){ // command for exit
            // Free the stack  
            while (cdStack->headOfStack != NULL){
                free(cdStack->headOfStack->dir);
                cdStack->headOfStack = cdStack->headOfStack->next;
            }
            fprintf(stderr, "Bye...\n"); // print out exit message 
            fprintCommand(currCommand.arguments[0], 0); // print out completed message 
            exit(0); // Exit from the shell prompt and program
        } 
        else{
            createProcess(&currCommand); // Now create process with current command 
        }
    }
    return 0;
}

int main(void){
    char cmd[CMDLINE_MAX]; // This will be our command line with a max of 512 
    char workdir[CMDLINE_MAX]; // This will be our working directory for creating cwd stack
    getcwd(workdir,CMDLINE_MAX); // Get the current working directory 
    
    /* Create our cwd stack */
    struct stack *cdStack = createNew(); // Initialize cwd stack
    if (cdStack->stackSize == 0){ //only runs once, initializes cwd in cdStack
        push(cdStack, workdir);
    }  
    int savedOut = dup(1), savedIn = dup(0); // Initialize a saved stdout and stdin

    /* This loop will prompt the user to enter a command to be executed until they want to exit */
    while (1) { 
        dup2(savedOut, 1); // Reset the stdout 
        dup2(savedIn, 0); // Reset the stdin 

        /* Print prompt */
        printf("sshell@ucd$ ");
        fflush(stdout);

        /* Get command line */
        fgets(cmd, CMDLINE_MAX, stdin);
        
        /* Print command line if stdin is not provided by terminal */
        if (!isatty(STDIN_FILENO)){
                printf("%s", cmd);
                fflush(stdout);
        }

        /* Remove trailing newline from command line */
        char * nl = strchr(cmd, '\n');
        if (nl){
                *nl = '\0';
        }
        
        // Check if user entered any arguments, if not then ask again
        if(cmd[0] == '\0'){
            fprintf(stderr, "Error: no arguments entered\n");
            continue;
        } 
        else {
            // Begin process by creating commands
            createCommand(cmd, cdStack);  
        }
    }
    return EXIT_SUCCESS;
} // end of program 