#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>


#define CMDLINE_MAX 512
#define ARGS_MAX 16

static inline void *xmalloc(size_t size) //professors provided project 0 implementation of linked list structs
{
    void *p = malloc(size);
    return p;
};
struct commandList{
    struct commandListNode *p, *p_tail;
    int numOfCommands;
};
struct commandListNode
{
    struct aCommand *command;
    struct commandListNode *next;
};

struct aCommand{
    char *processName;
    char *outputRedirFile;
    char *arguments[ARGS_MAX];
    int numOfArgs;
    int fd[2];
};

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

void printArgs(struct aCommand *currCommand){
    printf("Arguments in command obj: ");
    for (int i = 0; i < currCommand->numOfArgs; i++){
       printf("%s\t",currCommand->arguments[i]);
    }
    printf("\nNum args: %d\n", currCommand->numOfArgs);
}

void createProcess(struct aCommand *currCommand, char cmd[ARGS_MAX]){
    pid_t pid;
    pid = fork();
    int status;
        if (pid == 0) {
            // child
            execvp(currCommand->processName, currCommand->arguments); 
            perror("execvp");
            exit(1);
        } else if (pid > 0) {
            // parent
            waitpid(pid, NULL, 0);
        }else {
            perror("fork");
            exit(1);
        }
        // show status 
        fprintf(stderr, "+ completed '%s' [%d]\n",
                cmd, WEXITSTATUS(status));
}
void createProcessWithOutputRedirection(struct aCommand *currCommand, char cmd[ARGS_MAX]){
    pid_t pid;
    int status, toFileFD, savedSTDO;
    pid = fork();

    if (pid == 0) {
        // child
        toFileFD = open(currCommand->outputRedirFile, O_WRONLY | O_CREAT, 0644); //new fd
        savedSTDO = dup(STDOUT_FILENO); // save stdout
        dup2(toFileFD, STDOUT_FILENO); //replace stdout
        close(toFileFD);
        status = execvp(currCommand->processName, currCommand->arguments);
        perror("execvp");
        exit(1);
    } else if (pid > 0) {
        // parent
        //return to STDO
        waitpid(pid, NULL, 0);
        dup2(savedSTDO, toFileFD);
        close(savedSTDO);
    } else {
        perror("fork");
        exit(1);
    }
    fprintf(stderr, "+ completed '%s' [%d]\n",
            cmd, WEXITSTATUS(status)); //change to full command string later
}

void createPipes(struct commandList *cmdList, char cmd[ARGS_MAX]){

    struct commandListNode *access = xmalloc(sizeof(struct commandListNode));
    access = cmdList->p;
    // while (access != NULL){
    //     printArgs(access->command);
    //     access = access->next;
    // }  
    //  access = cmdList->p;

    pipe(access->command->fd); //pipe 1 A-B
    pipe(access->next->command->fd); //pipe 2 B-C

    pid_t pid;
    pid = fork();
    int status;

    if (pid !=  0) {
        // parent
        //fd of command 1
        //fd[1] is stout of command 1
        close(access->command->fd[0]);
        dup2(access->command->fd[1], STDOUT_FILENO);
        close(access->command->fd[1]);
        //printArgs(access->command);
        execvp(access->command->processName, access->command->arguments); 
        perror("execvp");
        exit(1);
    } else {
        // child
        //fd of command 2?
        //fd[0] is stdin of command2
        waitpid(pid, NULL, 0);
        close(access->command->fd[1]);
        dup2(access->command->fd[0], STDIN_FILENO);
        close(access->command->fd[0]);
        access = access->next;
        // printArgs(access->next->command);
        //execvp(access->next->command->processName, access->next->command->arguments); 
        execvp(access->command->processName, access->command->arguments); 
        perror("execvp");
        exit(1);
    }
    access = cmdList->p;
    close(access->command->fd[0]);
    close(access->command->fd[1]);
    access = access->next;
    close(access->command->fd[0]);
    close(access->command->fd[1]);
    // show status 
    fprintf(stderr, "+ completed '%s' [%d]\n",
            cmd, WEXITSTATUS(status));
}
void parseInPipes(struct commandList cmdList, char in[ARGS_MAX]){
        // parse in pipes
        // while != NULL 
                // while loop through num commands, 
                //      struct make curr command A B C D
                // while loop through cmd remaining, break out 
    struct aCommand currCommandOne; 
    memset(&currCommandOne, 0, sizeof(struct aCommand));
    struct aCommand currCommandTwo;
    memset(&currCommandTwo, 0, sizeof(struct aCommand));
    struct aCommand currCommandThree; 
    memset(&currCommandThree, 0, sizeof(struct aCommand));
    struct aCommand currCommandFour;
    memset(&currCommandFour, 0, sizeof(struct aCommand));

    char * token = strtok(in, " ");
    currCommandOne.arguments[0] = token;
    int argumentPos = 1, cmdItr = 1;
    while (token != NULL){
       // printf("tokens: %s\t%d\n",token, argumentPos);
        token = strtok(NULL, " ");
        if (token != NULL && (!strchr(token, '|'))){
            if (cmdItr == 1){
               //printf("in cmditr 1 statement\n");
                currCommandOne.arguments[argumentPos] = token;
                currCommandOne.processName = currCommandOne.arguments[0];
                argumentPos++;
                currCommandOne.arguments[argumentPos] = NULL;
                currCommandOne.numOfArgs = argumentPos;
              // printf("after add: %s\t%d\n",currCommandOne.arguments[argumentPos], argumentPos);
            }
            else if (cmdItr == 2){
                currCommandTwo.arguments[argumentPos] = token;
                currCommandTwo.processName = currCommandTwo.arguments[0];
                argumentPos++;
                currCommandTwo.arguments[argumentPos] = NULL;
                currCommandTwo.numOfArgs = argumentPos;
            }
            else if (cmdItr == 3){
                currCommandThree.arguments[argumentPos] = token;
                currCommandThree.processName = currCommandThree.arguments[0];
                argumentPos++;
                currCommandThree.arguments[argumentPos] = NULL;
                currCommandThree.numOfArgs = argumentPos;
            }
             else if (cmdItr == 4) {
                currCommandFour.arguments[argumentPos] = token;
                currCommandFour.processName = currCommandFour.arguments[0];
                argumentPos++;
                currCommandFour.arguments[argumentPos] = NULL;
                currCommandFour.numOfArgs = argumentPos;
            }
        }
        else{//housekeeping
            argumentPos = 0;
            cmdItr++;
        }
    }
    
    for (int i = 0; i < cmdList.numOfCommands; i++){
        struct commandListNode *p = xmalloc(sizeof(struct commandListNode));
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
    //  struct commandListNode *print = xmalloc(sizeof(struct commandListNode));
    //  print = cmdList.p;
    // while (print != NULL){
    //     printArgs(print->command);
    //     print = print->next;
    // }  
    
    createPipes(&cmdList, in);
}
void parseIn(struct aCommand *currCommand, char in[ARGS_MAX]){
    char * token = strtok(in, " ");
    currCommand->arguments[0] = token;
    currCommand->processName = token;
    int argumentPos = 1;
    while (token != NULL) { // while we still have tokens left in cmd, cont
        //printf("token1: %s\t\n",token);
        token = strtok(NULL, " ");
        if (token != NULL)
        {
            //printf("\ninside else if \n");
            // add token to currCommand arguments @mu
            currCommand->arguments[argumentPos] = token;
            // printf("token2: %s\t%d\n",currCommand->arguments[argumentPos], argumentPos);
            if(strchr(token, '>')){
               // printf("\ninside if > \n");
                currCommand->arguments[argumentPos] = NULL;
                token = strtok(NULL, " ");
                currCommand->outputRedirFile = token;
                break;
            }

         argumentPos++;

        }
        //printf("token2: %s\t%d\n",token, argumentPos);
            
    }

    currCommand->arguments[argumentPos++] = NULL;
    currCommand->numOfArgs = argumentPos; //final number of args
}
int createCommand(char cmd[]){
    char cmdCopy[CMDLINE_MAX];
    strncpy(cmdCopy, cmd, CMDLINE_MAX);

    struct commandList cmdList; //command 1
    memset(&cmdList, 0, sizeof(struct commandList));
    cmdList.numOfCommands = 0;

    struct aCommand currCommand; //command 1
    memset(&currCommand, 0, sizeof(struct aCommand));

    // parseIn(&currCommand, cmd);
    //if(strchr(cmd, '|') || strchr(cmd, '<') || strchr(cmd, '>')){
    if(strchr(cmd, '|')){
        // Method here to check number of commands
        cmdList.numOfCommands = getNumPipeCommands(cmdCopy);
       // printf("\t%d\n",cmdList.numOfCommands);
        parseInPipes(cmdList, cmd);
        
        // parse in pipes
        // while != NULL 
                // while loop through num commands, 
                //      struct make curr command A B C D
                // while loop through cmd remaining, break out 
        
        // printf("%s\t\n",cmd);
        // struct commandListNode *print = xmalloc(sizeof(struct commandListNode));
        // print = cmdList.p;
        // while (print != NULL)
        // {
        //     // printf("arg0: %s\n",print->command->processName);
        //     printArgs(print->command);
        //     print = print->next;
        // }  
       // printf("inside pipe create command\n");
    }
    else if(strchr(cmd, '>'))
    {
        parseIn(&currCommand, cmd);
        printArgs(&currCommand);
        printf("output file: %s\n",currCommand.outputRedirFile);
        createProcessWithOutputRedirection(&currCommand, cmdCopy);
        
        // printf("outside output");
        /* code */
    }
    else{
        parseIn(&currCommand, cmd);
        printArgs(&currCommand);
        createProcess(&currCommand, cmdCopy);
    }
}
int main(void)
{
        char cmd[CMDLINE_MAX];
        while (1) {
                char *nl;
                char *arguments[16];
                int inc = 1;

                /* Print prompt */
                printf("sshell@ucd$ ");
                fflush(stdout);

                /* Get command line */
                fgets(cmd, CMDLINE_MAX, stdin);
                
                /* Print command line if stdin is not provided by terminal */
                if (!isatty(STDIN_FILENO)) {
                        printf("%s\n", cmd);
                        fflush(stdout);
                }

                /* Remove trailing newline from command line */
                nl = strchr(cmd, '\n');
                if (nl) {
                        *nl = '\0';
                }
                /* Builtin command */
                // this is the command for exit
                if (!strcmp(cmd, "exit")) {
                        fprintf(stderr, "Bye...\n");
                        break;
                } 
                // the is the command for pwd 
                if (!strcmp(cmd, "pwd")) {
                        char workdir[CMDLINE_MAX];
                        printf("CURRENT DIR: %s\n", getcwd(workdir, CMDLINE_MAX));
                        continue;
                } 
                //printArgs(&currCommand);
                //createProcess(&currCommand, cmdCopy);
                createCommand(cmd);  
        }

        return EXIT_SUCCESS;
}
