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

struct aCommand{
    char * processName;
    char *arguments[ARGS_MAX];
    int numOfArgs;
    char *outputRedirFile;
};

void printArgs(struct aCommand *currCommand){
    printf("Arguments in command obj: ");
    for (size_t i = 0; i < currCommand->numOfArgs; i++){
       printf("%s\t",currCommand->arguments[i]);
    }
    printf("\nNum args: %d\n", currCommand->numOfArgs);
}

void parseIn(struct aCommand *currCommand, char in[ARGS_MAX]){
    char * token = strtok(in, " ");
    currCommand->processName = token;
    currCommand->arguments[0] = token;
    int argumentPos = 1;
    while (token != NULL) { // while we still have tokens left in cmd, cont
       // printf("token1: %s\t\n",token);
        token = strtok(NULL, " ");
        if (token != NULL)
        {
            //printf("\ninside else if \n");
            // add token to currCommand arguments @mu
            currCommand->arguments[argumentPos] = token;
            if(strchr(token, '>')){
               // printf("\ninside if > \n");
                currCommand->arguments[argumentPos] = NULL;
                token = strtok(NULL, " ");
                currCommand->outputRedirFile = token;
                break;
            }
            else if (strchr(token, '|'))
            {
                //push curr object to object array/struct/lined list maybe return it and recursive call/call parseIn again if it returns an obj until it returns null from createCommand
                //or reset entire currCommand after saving it somewhere
                //continue looping through rest of tokens until empty
                //argumentPos = 0; //reset numOfArgs for new currCommand obj
                // currCommand->arguments[argumentPos++] = NULL;
                // currCommand->numOfArgs = argumentPos; //final number of args

            }
            
            numArgs++;
        }
       // printf("token2: %s\t%d\n",token, numArgs);
    }
    currCommand->arguments[argumentPos] = NULL;
    currCommand->numOfArgs = argumentPos; //final number of args
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
// void createPipes(){

// }
int createCommand(char cmd[]){
    char cmdCopy[CMDLINE_MAX];
    strncpy(cmdCopy, cmd, CMDLINE_MAX);
    struct aCommand currCommand; //command 1
    memset(&currCommand, 0, sizeof(struct aCommand));
    // parseIn(&currCommand, cmd);
    //if(strchr(cmd, '|') || strchr(cmd, '<') || strchr(cmd, '>')){
    if(strchr(cmd, '|')){
        parseIn(&currCommand, cmd);
        printf("inside pipe\n");
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
       // printArgs(&currCommand);
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
                        printf("%s", cmd);
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
