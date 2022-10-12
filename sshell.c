#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>



#define PATH_MAX 4096
#define CMDLINE_MAX 512
#define ARGS_MAX 16

static inline void *xmalloc(size_t size){ //professors provided project 0 implementation of linked list structs

    void *p = malloc(size);
    return p;
};
//print command exit status to stderr
void fprintCommandStatus(char cmd[CMDLINE_MAX], int status){
    fprintf(stderr, "+ completed '%s' [%d]\n",
                cmd, WEXITSTATUS(status));
}
//print builtin and simple command exit int value to stderr
void fprintCommand(char cmd[CMDLINE_MAX], int status){
    fprintf(stderr, "+ completed '%s' [%d]\n",
                cmd, status);
}
//linked list used to house 2-4 commands used for piping, currenly hardcoded for 2-4 but scaleability possible
struct commandList{
    struct commandListNode *p, *p_tail;
    int numOfCommands;
};
struct commandListNode{
    struct aCommand *command;
    struct commandListNode *next;
};

struct aCommand{
    char *processName;
    char *outputRedirFile;
    char *arguments[ARGS_MAX];
    char cmdSave[CMDLINE_MAX];
    int numOfArgs;
    int fd[2];
};
//command print for debug
void printArgs(struct aCommand *currCommand){
    printf("Arguments in command obj: ");
    for (int i = 0; i < currCommand->numOfArgs; i++){
       printf("%s\t",currCommand->arguments[i]);
    }
    printf("\nNum args: %d\n", currCommand->numOfArgs);
}
//pushd/popd/dir stack struct/struct funcs
//referenced https://stackoverflow.com/questions/1919975/creating-a-stack-of-strings-in-c for structure of a single link stack 
struct stack{ //stack
  struct stackNode *headOfStack;
  int stackSize;
};
struct stackNode{ //stack entyr
  char *dir;
  struct stackNode *next;
};
struct stack *createNew(void){
  struct stack *stackRet = (stackRet*)xmalloc(sizeof(*stackRet));
  if (stackRet){ //if successful malloc initialize stack vars
    stackRet->headOfStack = NULL;
    stackRet->stackSize = 0;
  }
  return stackRet;
};
//helper stack funcs
void push(struct stack *currStack, char* dirToPush){
    struct stackNode * stackToEnter = (stackNode*)xmalloc(sizeof *stackToEnter);
    char * copy = (char *)malloc(strlen(dirToPush) + 1); 
    if (stackToEnter){ //if it exists
        strcpy(copy, dirToPush); //mem leak fix how?
        stackToEnter->dir = copy;
        stackToEnter->next = currStack->headOfStack;
        currStack->headOfStack = stackToEnter;
       // printf("cur dir: %s\n", currStack->headOfStack->dir);
        currStack->stackSize++;
    }
}
void pop(struct stack *currStack){
    if (currStack->headOfStack != NULL && ((currStack->stackSize-1) != 0)){
        chdir(currStack->headOfStack->next->dir);
        free(currStack->headOfStack->dir);
        currStack->headOfStack = currStack->headOfStack->next;
        currStack->stackSize--;
        fprintCommand((char *)"popd", 0);
    }
    else{
        fprintf(stderr, "Error: directory stack empty\n");
        fprintCommand((char*)"popd", 1);
        return;
    }
}

int getNumPipeCommands(char cmd[ARGS_MAX]){
    int numCommands = 1;
    char cmdCopy[CMDLINE_MAX];
    char* n1;
    strncpy(cmdCopy, cmd, CMDLINE_MAX);
    while (1){
        n1 = strchr(cmdCopy, '|');
        if (n1)
        {
            *n1 = ' '; 
            numCommands++;
        }
        else{
            break;
        }
    }
    return numCommands;
}

void createProcess(struct aCommand *currCommand){
    pid_t pid;
    pid = fork();
    int status = -1;
        if (pid == 0) {
            // child
            execvp(currCommand->processName, currCommand->arguments); 
            perror("execvp");
            exit(1);
        } 
        else if (pid > 0) {
            // parent
            waitpid(pid, &status, 0);
        }
        else {
            perror("fork");
            exit(1);
        }
        // show status 
        // fprintf(stderr, "+ completed '%s' [%d]\n",
        //         currCommand->cmdSave, WEXITSTATUS(status));
        fprintCommandStatus(currCommand->cmdSave, status);
}
void createProcessWithOutputRedirection(struct aCommand *currCommand){
    pid_t pid;
    pid = fork();
    int status = -1, toFileFD = 0, savedSTDO = 0;
    if (pid == 0) {
        // child
        // check if the output file is NULL 
        if(currCommand->outputRedirFile == NULL){//catch no output file
            fprintf(stderr, "Error: no output file\n");
            return;
        }
        toFileFD = open(currCommand->outputRedirFile, O_WRONLY | O_CREAT, 0644); //new fd
        savedSTDO = dup(STDOUT_FILENO); // save stdout
        dup2(toFileFD, STDOUT_FILENO); //replace stdout
        close(toFileFD);
        execvp(currCommand->processName, currCommand->arguments);
        perror("execvp");
        exit(1);
    } 
    else if (pid > 0) {
        // parent
        //return to STDO
        waitpid(pid, &status, 0);
        fflush(stdout);
        dup2(savedSTDO, toFileFD);
        close(savedSTDO);
    } 
    else {
        perror("fork");
        exit(1);
    }
    fprintCommandStatus(currCommand->cmdSave, status);
}

void createPipes(struct commandList *cmdList){
    struct commandListNode *access = (commandListNode*)xmalloc(sizeof(struct commandListNode));
    access = cmdList->p;

  int savedOut = dup(1);
    //struct commandListNode *print = xmalloc(sizeof(struct commandListNode));
    // print = cmdList->p;
    // while (print != NULL){
    //     printArgs(print->command);
    //     print = print->next;
    // }  

    if (cmdList->numOfCommands == 2){
        pipe(access->command->fd); //pipe 1 A-B
        pid_t pid1, pid2;
        int pid1Status, pid2Status;
        switch (pid1 = fork()){
            case -1:
                /* code */
                break;
            case 0: //child
                //change stdout fd[1]
                close(access->command->fd[0]);
                dup2(access->command->fd[1], STDOUT_FILENO);
                close(access->command->fd[1]);
                execvp(access->command->processName, access->command->arguments); 
            default:
                break;
        }
        switch (pid2 = fork()){
            case -1:
                /* code */
                break;
            case 0: //child
            
                //change stdout fd[0]
                close(access->command->fd[1]);
                dup2(access->command->fd[0], STDIN_FILENO);
                close(access->command->fd[0]);
                execvp(access->next->command->processName, access->next->command->arguments); 
            default:
                break;
        }
            fflush(stdout);
            close(access->command->fd[0]);
            close(access->command->fd[1]);
            waitpid(pid1, &pid1Status,  0);
            waitpid(pid2, &pid2Status,  0);
            fprintf(stderr, "+ completed '%s' [%d][%d]\n",
                    access->command->cmdSave, WEXITSTATUS(pid1Status), WEXITSTATUS(pid2Status));
    }
    else if (cmdList->numOfCommands == 3){
        pipe(access->command->fd); //pipe 1 A-B
        pipe(access->next->command->fd); //pipe 2 B-C
        pid_t pid1, pid2, pid3;
        int pid1Status, pid2Status, pid3Status;
        switch (pid1 = fork()){
            case -1:
                /* code */
                break;
            case 0: //child
                close(access->command->fd[0]);
                dup2(access->command->fd[1], STDOUT_FILENO);
                close(access->command->fd[1]);
                execvp(access->command->processName, access->command->arguments); 
            default:
                break;
        }
        switch (pid2 = fork()){
            case -1:
                /* code */
                break;
            case 0: //child
                close(access->command->fd[1]);
                dup2(access->command->fd[0], STDIN_FILENO);
                close(access->command->fd[0]);

                close(access->next->command->fd[0]);
                dup2(access->next->command->fd[1], STDOUT_FILENO);
                close(access->next->command->fd[1]);
                execvp(access->next->command->processName, access->next->command->arguments); 
            default:
                break;
        }
        switch (pid3 = fork()){
            case -1:
                /* code */
                break;
            case 0: //child
                close(access->next->command->fd[1]);
                dup2(access->next->command->fd[0], STDIN_FILENO);
                close(access->next->command->fd[0]);
                //printArgs(access->next->next->command);
                execvp(access->next->next->command->processName, access->next->next->command->arguments); 
            default:
                break;
        }
            dup2(savedOut, 1);
            close(access->next->command->fd[0]);
            close(access->next->command->fd[1]);

            close(access->command->fd[0]);
            close(access->command->fd[1]);

            waitpid(pid1, &pid1Status,  0);
            waitpid(pid2, &pid2Status,  0);
            waitpid(pid3, &pid3Status,  0);

            fprintf(stderr, "+ completed '%s' [%d][%d][%d]\n",
                    access->command->cmdSave, WEXITSTATUS(pid1Status), WEXITSTATUS(pid2Status), WEXITSTATUS(pid3Status));
    }
    // else if (cmdList->numOfCommands == 4){
    //     pipe(access->command->fd); //pipe 1 A-B
    //     pipe(access->next->command->fd); //pipe 2 B-C
    //     pipe(access->next->next->command->fd); // pipe 3 C-D
    //     pid_t pid1, pid2, pid3, pid4;
    //     int pid1Status, pid2Status, pid3Status, pid4Status;
    //     switch (pid1 = fork()){
    //         case -1:
    //             /* code */
    //             break;
    //         case 0: //child
    //             close(access->command->fd[0]);
    //             dup2(access->command->fd[1], STDOUT_FILENO);
    //             close(access->command->fd[1]);
    //             execvp(access->command->processName, access->command->arguments); 
    //         default:
    //             break;
    //     }
    //     switch (pid2 = fork()){
    //         case -1:
    //             /* code */
    //             break;
    //         case 0: //child
    //             close(access->command->fd[1]);
    //             dup2(access->command->fd[0], STDIN_FILENO);
    //             close(access->command->fd[0]);

    //             close(access->next->command->fd[0]);
    //             dup2(access->next->command->fd[1], STDOUT_FILENO);
    //             close(access->next->command->fd[1]);
    //             execvp(access->next->command->processName, access->next->command->arguments); 
    //         default:
    //             break;
    //     }
    
    //     switch (pid3 = fork()){
    //         case -1:
    //             /* code */
    //             break;
    //         case 0: //child
    //             close(access->next->command->fd[1]);
    //             dup2(access->next->command->fd[0], STDIN_FILENO);
    //             close(access->next->command->fd[0]);

    //             close(access->next->next->command->fd[0]);
    //             dup2(access->next->next->command->fd[1], STDOUT_FILENO);
    //             close(access->next->next->command->fd[1]);
    //             execvp(access->next->next->next->command->processName, access->next->next->next->command->arguments); 
    //         default:
    //             break;
    //     }
    //       switch (pid4 = fork()){
    //         case -1:
    //             /* code */
    //             break;
    //         case 0: //child
    //             close(access->next->command->fd[1]);
    //             dup2(access->next->command->fd[0], STDIN_FILENO);
    //             close(access->next->command->fd[0]);
    //             execvp(access->next->next->command->processName, access->next->next->command->arguments); 
    //         default:
    //             break;
    //     }

            // close(access->command->fd[0]);
            // close(access->command->fd[1]);

            // close(access->next->command->fd[0]);
            // close(access->next->command->fd[1]);

            // close(access->next->next->command->fd[0]);
            // close(access->next->next->command->fd[1]);
    
                  
            // waitpid(pid1, &pid1Status,  0);
            // waitpid(pid2, &pid2Status,  0);
            // waitpid(pid3, &pid3Status,  0);
            // waitpid(pid4, &pid4Status,  0);
            // fprintf(stderr, "+ completed '%s' [%d][%d][%d][%d]\n",
            //         access->command->cmdSave, WEXITSTATUS(pid1Status), WEXITSTATUS(pid2Status), WEXITSTATUS(pid3Status), WEXITSTATUS(pid4Status));
    //}
}
    
void parseInPipes(struct commandList cmdList, char cmd[CMDLINE_MAX]){

    struct aCommand currCommandOne; 
    memset(&currCommandOne, 0, sizeof(struct aCommand));
    struct aCommand currCommandTwo;
    memset(&currCommandTwo, 0, sizeof(struct aCommand));
    struct aCommand currCommandThree; 
    memset(&currCommandThree, 0, sizeof(struct aCommand));
    struct aCommand currCommandFour;
    memset(&currCommandFour, 0, sizeof(struct aCommand));


    int argumentPos = 1, cmdItr = 1;
    strncpy(currCommandOne.cmdSave, cmd, CMDLINE_MAX); //saves initial untokenized cmdIn
    char * token = strtok(cmd, " "); // take initial token
    currCommandOne.arguments[0] = token; //set initial arg0
    currCommandOne.processName = currCommandOne.arguments[0]; // set process name for command 1
    while (token != NULL){
    //    printf("\ttokens: %s\t%d\n",token, argumentPos);
        token = strtok(NULL, " ");
        if (token != NULL && (!strchr(token, '|'))){ //token is confirmed not null and is not the pipe character signifying a new command will follow
            if (cmdItr == 1){
                currCommandOne.arguments[argumentPos] = token;
                //printf("after add: %s\t%d",currCommandOne.arguments[argumentPos], argumentPos);
                argumentPos++;
                currCommandOne.numOfArgs = argumentPos;
            //   printf("b4 add: %s\t%d\n",currCommandOne.arguments[argumentPos], argumentPos);
            }
            else if (cmdItr == 2){
                currCommandTwo.arguments[argumentPos] = token;
                currCommandTwo.processName = currCommandTwo.arguments[0];
                argumentPos++;
                currCommandTwo.numOfArgs = argumentPos;
            }
            else if (cmdItr == 3){
                currCommandThree.arguments[argumentPos] = token;
                currCommandThree.processName = currCommandThree.arguments[0];
                argumentPos++;
                currCommandThree.numOfArgs = argumentPos;
            }
            else if (cmdItr == 4){
                currCommandFour.arguments[argumentPos] = token;
                currCommandFour.processName = currCommandFour.arguments[0];
                argumentPos++;
                currCommandFour.numOfArgs = argumentPos;
            }
        }
        else{//move and prep for next command
            currCommandOne.arguments[argumentPos+2] = NULL;//set last arg as null for execvp requirments
            argumentPos = 0;//reset argpost for next command
            cmdItr++;//move to next command
        }
    }

    for (int i = 0; i < cmdList.numOfCommands; i++){
        struct commandListNode *p = (commandListNode*)xmalloc(sizeof(struct commandListNode));
        if (i == 0){
            p->command = &currCommandOne;
        }
        else if (i == 1){
            p->command = &currCommandTwo;
        }
        else if (i == 2){
            p->command = &currCommandThree;
        }
        else if (i == 3){
            p->command = &currCommandFour;
        }
        p->next = NULL;
       // printArgs(cmdList.p->command->arguments);
        if (!cmdList.p){
            cmdList.p = p;
            cmdList.p_tail = p;
             //  printArgs(cmdList.p->command);
        }
        else{
            cmdList.p_tail->next = p;
            cmdList.p_tail = p;
               // printArgs(cmdList.p->next->command);
        }
    }
    //struct commandListNode *print = xmalloc(sizeof(struct commandListNode));
    //  print = cmdList.p;
    // while (print != NULL){
    //     printArgs(print->command);
    //     print = print->next;
    // }  
    
    createPipes(&cmdList);
}
void parseIn(struct aCommand *currCommand, char cmd[CMDLINE_MAX]){
    strncpy(currCommand->cmdSave, cmd, CMDLINE_MAX); //saves initial untokenized cmdIn
    char * token = strtok(cmd, " "); //parse initial token
    currCommand->arguments[0] = token; //set up initial arg0
    currCommand->processName = token; //processName = arg0
    //currCommand->outputRedirFile = NULL;
    int argumentPos = 1;
    char *replace;
    while (token != NULL){ // while we still have tokens left in cmd, cont
        token = strtok(NULL, " ");
        if (token != NULL){
            // add token to currCommand arguments @mu
            currCommand->arguments[argumentPos] = token;
            if((replace = strchr(token, '>'))){
                currCommand->arguments[argumentPos] = NULL; //erase > char in args
                token = strtok(NULL, " "); // take new token which will have fileout
                currCommand->outputRedirFile = token;
                break;
            }
            argumentPos++;
        }   
    }
    currCommand->arguments[argumentPos++] = NULL;
    currCommand->numOfArgs = argumentPos; //final number of args
}

int createCommand(char cmd[], struct stack *cdStack){
    char workdir[CMDLINE_MAX];//variables used in creating cwd stack
    getcwd(workdir,CMDLINE_MAX);     
    struct commandList cmdList; //command 1
    memset(&cmdList, 0, sizeof(struct commandList));
    cmdList.numOfCommands = 0;

    struct aCommand currCommand; //command 1
    memset(&currCommand, 0, sizeof(struct aCommand));

    // parseIn(&currCommand, cmd);
    //if(strchr(cmd, '|') || strchr(cmd, '<') || strchr(cmd, '>')){
    if(strchr(cmd, '|')){
        // Method here to check number of commands
        cmdList.numOfCommands = getNumPipeCommands(cmd);
       // printf("\t%d\n",cmdList.numOfCommands);
        parseInPipes(cmdList, cmd);
    }
    else if(strchr(cmd, '>')){  
        parseIn(&currCommand, cmd);
            if (strchr(currCommand.arguments[0], '>')){
                fprintf(stderr,"Error: Missing command\n");
                return 0;
            }
 
            
        //printArgs(&currCommand);
        //printf("output file: %s\n",currCommand.outputRedirFile);
        createProcessWithOutputRedirection(&currCommand);
        return 0;
    }
    else{
        parseIn(&currCommand, cmd);
       // printArgs(&currCommand);
        if (currCommand.numOfArgs > 15){
            fprintf(stderr,"Error: too many process arguments\n");
            return 0;
        }

        //built in cmds
        if (!strcmp(currCommand.arguments[0], "exit")){
            fprintf(stderr, "Bye...\n");
            fprintCommand(currCommand.arguments[0], 0);
            exit(0);
        } 
        else if (!strcmp(currCommand.arguments[0], "pushd")){
            if (currCommand.arguments[1] != NULL) {
                chdir(currCommand.arguments[1]);
            }
            else{// cd no target dir
                chdir(getenv("HOME"));
            }
            getcwd(workdir,CMDLINE_MAX);
            push(cdStack, workdir);
            fprintCommand(currCommand.cmdSave, 0);
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
        else if (!strcmp(currCommand.arguments[0], "cd")) { 
            chdir(currCommand.arguments[1]);
            fprintCommand(currCommand.cmdSave, 0);
        }
        else if (!strcmp(currCommand.arguments[0], "pwd")){
            printf("%s\n", workdir);
            fprintCommand(currCommand.cmdSave, 0);
        } 
        else{
            createProcess(&currCommand);
        }
    }

    return 0;
}
int main(void){
    char cmd[CMDLINE_MAX];
    char workdir[CMDLINE_MAX];//variables used in creating cwd stack
    getcwd(workdir,CMDLINE_MAX); 
    struct stack *cdStack = createNew(); //initialize cwd stack
    if (cdStack->stackSize == 0){ //only runs once, initializes cwd in cdStack
        push(cdStack, workdir);
    }  
    int savedOut = dup(1), savedIn = dup(0);
    while (1) {
        dup2(savedOut, 1);
        dup2(savedIn, 0);
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
        /* Builtin command */
        // this is the command for exit
        // the is the command for pwd 
        //printArgs(&currCommand);
        //createProcess(&currCommand, cmdCopy);
        // Check if user entered any arguments, if not then ask again
        if(cmd[0] == '\0'){
            fprintf(stderr, "Error: no arguments entered\n");
            continue;
        } 
        else {
            createCommand(cmd, cdStack);  
        }
    }
        return EXIT_SUCCESS;
}