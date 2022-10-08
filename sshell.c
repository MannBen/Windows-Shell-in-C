#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <stdbool.h>

#define CMDLINE_MAX 512

//cd /home/cs150jp/public/p1/

// making a struct which will make an array of string args to char args*[] for execvp
// struct cmdArgs{
//        char *arguments[16];
// }

int argHandler(char *[], char [], char[]);
void createProcess(char *[], char *[]);
void createProcessWithOutputRedirection(char *[], char*);
void createPipes(char *[], char*[]);

int main(void)
{
    // Initialize variables for cmd line,
    // work directory and an array to parse arguments
    char cmd[CMDLINE_MAX], cmdCopy[CMDLINE_MAX];
    char workdir[CMDLINE_MAX];
    char *arguments[16];

    while (1) {
        char *nl;       // variable for getting rid of new line from fgets
        int inc = 1;    // increment variable for adding to arguments array

        /* Print prompt */
        printf("sshell@ucd$ ");
        fflush(stdout);

        /* Get command line */
        fgets(cmd, CMDLINE_MAX, stdin);
        //printf("cmd raw: %s\n", cmd);

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
        strncpy(cmdCopy, cmd, CMDLINE_MAX);
        // Check if user entered any arguments, if not then ask again
        if(cmd[0] == '\0') {
            fprintf(stderr, "Error: no arguments entered\n");
            continue;
        } else {
            // Parse arguments into array
            //add initial raw token
            char *token = strtok(cmd, " ");
            arguments[0] = token;

            while (token != NULL) { // while we still have tokens left in cmd, cont
                //printf("token1: %s\t%d\n",token, inc);
                token = strtok(NULL, " ");
                arguments[inc] = token;// add token to arguments
                inc++;
                // printf("token2: %s\t%d\n",token, inc)
            }

            // Check if valid process arguments
            if (inc == 18) {
                fprintf(stderr, "Error: too many process arguments\n");
            } else {
                // if valid arguments, then handle the proccesses
                //printf("cmd raw: %s\n", cmd);
                argHandler(arguments, workdir, cmdCopy);
            }
        }
    }

    return EXIT_SUCCESS;
}

int argHandler(char* arguments[], char workdir[], char cmdC[]) {
    int currPos = 0;
    char *cleanedArguments[16];
    char *outRedirect, *pipe, *inRedirect;
    bool hasChar = false;
    //   printf("cmdc: %s\t",cmdC);
    while(arguments[currPos] != NULL){
        // initialize special characters
        outRedirect = strchr(arguments[currPos], '>');
        pipe = strchr(arguments[currPos], '|');
        inRedirect = strchr(arguments[currPos], '<');
        //if not < > >&  | add to cleanargs
        // If not a > < or | then go in if statement
        if(outRedirect == 0 && pipe == 0 && inRedirect == 0){ //parse args into cleaned args if not <>|
            cleanedArguments[currPos] = arguments[currPos];
            //printf("args: %s\t%d\n", cleanedArguments[currPos], currPos);
            currPos++;
        } else if(outRedirect != 0) {
            if(arguments[currPos+1] == NULL){//catch no output file
                fprintf(stderr, "Error: no output file\n");
                return 0;
            }
            // this will handle the > output redirection
            printf("In carrot statement\n");
            hasChar = true;
            cleanedArguments[currPos] = NULL;
            createProcessWithOutputRedirection(cleanedArguments, arguments[currPos+1]);
            currPos += 2;
            break; // this will then break out of the else and continue going through argumetns
        } else if(inRedirect != 0) {
            printf("In < Statement\n");
            hasChar = true;
            //then handle input redirection <
            break;
        } else if (pipe != 0) {
            // check for pipe
            // can be up to 3 pipe signs on the same command line
            // process 1 | process 2 | process 3
            hasChar = true;
            printf("In | Statement\n");
            createPipes(cleanedArguments, arguments);
            break;
        }
    }

    // /* Builtin commands */
    // //This will be the exit command
    if (!strcmp(cleanedArguments[0], "exit")) {
        fprintf(stderr, "Bye...\n");
        exit(0);
    }
        // the is the command for pwd
    else if (!strcmp(cleanedArguments[0], "pwd")) {
        printf("CURRENT DIR: %s\n", getcwd(workdir, CMDLINE_MAX));
    }
        // this is the command for cd
    else if (!strcmp(cleanedArguments[0], "cd")) {
        chdir(cleanedArguments[1]);
    }
        // this will create a process if string doesnt have a > < or | remaining
    else if (!hasChar) {
        cleanedArguments[currPos] = NULL;
        createProcess(arguments, cleanedArguments);
    }
    return 0;
}

void createProcess(char* arguments[], char *cleanedArguments[]){
    pid_t pid;
    pid = fork();
    int status;
    if (pid == 0) {
        execvp(cleanedArguments[0], cleanedArguments);
        perror("execvp");
        exit(1);
    } else if (pid > 0) {
        // parent
        waitpid(pid, &status, 0);
    } else {
        perror("fork");
        exit(1);
    }
    // print full command string output
    int j = 0;
    fprintf(stderr, "+ completed '");
    while (arguments[j] != NULL) {
        fprintf(stderr, "%s", arguments[j]);
        j++;
        if(arguments[j] == NULL) {
            break;
        } else {
            fprintf(stderr, " ");
        }
    }
    fprintf(stderr, "' [%d]\n", WEXITSTATUS(status));
//    fprintf(stderr, "+ completed '%s' [%d]\n",
//            cleanedArguments[0], WEXITSTATUS(status)); //change to full command string later
}


void createProcessWithOutputRedirection(char *cleanedArguments[], char* outputFile){
    pid_t pid;
    pid = fork();
    int status, toFileFD, savedSTDO;

    if (pid == 0) {
        // child
        toFileFD = open(outputFile, O_WRONLY | O_CREAT, 0644); //new fd
        savedSTDO = dup(STDOUT_FILENO); // save stdout
        dup2(toFileFD, STDOUT_FILENO); //replace stdout
        close(toFileFD);
        execvp(cleanedArguments[0], cleanedArguments);
        perror("execvp");
        exit(1);
    } else if (pid > 0) {
        // parent
        waitpid(pid, &status, 0);
        //return to STDO
        dup2(savedSTDO, toFileFD);
        close(savedSTDO);
    } else {
        perror("fork");
        exit(1);
    }

    fprintf(stderr, "+ completed '%s' [%d]\n",
            cleanedArguments[0], WEXITSTATUS(status)); //change to full command string later
}

void createPipes(char *cleanedArguments[], char *remainingArguments[]){
    pid_t pid;
    int status, currPos = 0, numArgs = 1, temp = 0 ;
    int fdEdge[2], fdMiddle[2];
    char *perpipe[16];

    // echo hello World | Test | test > file
    while(remainingArguments[currPos] != NULL){
        //    printf("args in pipes cleaned : %s\t%d\n", cleanedArguments[currPos], currPos);
        if(!strcmp(remainingArguments[currPos], "|")){
            numArgs++;
        }
        currPos++;
    }
    // printf("%d\n", numArgs);

    for(int i = 0; i < numArgs; i++) {
        currPos = 0;
        while(remainingArguments[temp] != NULL && strcmp(remainingArguments[temp], "|")){
            perpipe[currPos] = remainingArguments[temp];
            printf("args in perpipe : %s\t%d\n", perpipe[currPos], currPos);
            currPos++;
            temp++;
        }
        perpipe[currPos] = NULL;
        temp++;

        pipe(fdEdge);
        pid = fork();

        if (pid == 0) { // child
            //am i command 1 and 2
            //if 1 pipe out to 2
            //if 2 pine in from 1 and out to console.tm (unless i am a middle child and need to pipe to another process or am output redirecting to a file)
            //code reviewers divert your eyes
            if(i == 0){ // A
                close(fdEdge[0]);
                //printf("test1:  [%d]\n", fdEdge[1]);
                dup2(fdEdge[1], STDOUT_FILENO);
                // dup2(fdEdge[0], STDIN_FILENO);
                // need to do a STDIN_FILENO    
                close(fdEdge[1]);
                     //change to full command string later
            }
            else if(i == 1 && numArgs == 2){ //if is B command and the last command A - B
                //i dont understand
                //fd edge 0 = 3
                 //fd edge 1 = 4
                printf("test:  [%d]\n", fdEdge[0]);
                printf("test:  [%d]\n", fdEdge[1]);
                close(fdEdge[1]);
                dup2(fdEdge[0], STDIN_FILENO);
                close(fdEdge[0]);
                 //fd edge 0 = 4
                 //fd edge 1 = 3
                //  close(fdEdge[0]);
                // dup2(fdEdge[1], STDIN_FILENO);
                // close(fdEdge[1]);
                printf("test2:  [%d]\n", fdEdge[0]);
                printf("test2:  [%d]\n", fdEdge[1]);
                    
            }
            // else if(i == 1){ / if is B command and is not the last, am i 2nd to last A - B - C, or am i 3d to last A - B - C - D

            // }
            // else if(i == 2 && numArgs == 3){ //if is C command and the last command A - B - C or A - B - C - D

            // }
            execvp(perpipe[0], perpipe);
            perror("execvp");
            exit(1);
        } 
        else if(pid > 0){
                        // parent
           if(i == 0){
                // printf("testa:  [%d]\n", fdEdge[0]);
                // printf("testa:  [%d]\n", fdEdge[1]);
                close(fdEdge[1]);
                close(fdEdge[0]);
           }
           if(i == 1){
            //   printf("testb:  [%d]\n", fdEdge[0]);
            //     printf("testb:  [%d]\n", fdEdge[1]);
                close(fdEdge[1]);
                close(fdEdge[0]);
           }
           waitpid(pid, &status, 0);
        }
        else{
            perror("fork");
            exit(1);
        }
            fprintf(stderr, "+ completed '%s' [%d]\n",
                perpipe[0], WEXITSTATUS(status)); //change to full command string later
    }
/*
    pipe(fdEdge);
    if (fork != 0) {
        close(fdEdge[0]);
        dup2(fdEdge[1], STDOUT_FILENO);
        close(fdEdge[1]);
        execvp(perpipe[0], perpipe);
    } else {
        close(fdEdge[1]);
        dup(fdEdge[0], STDIN_FILENO);
        close(fdEdge[0]);
        execvp()
    }
*/
    // currPos = 0;
    // while(remainingArguments[currPos] != NULL){
    //    printf("args in pipes remaining : %s\t%d\n", remainingArguments[currPos], currPos);
    //    currPos++;
    // }
    // printf("args in pipes cleaned : %s\t%d\n", cleanedArguments[currPos], currPos);

}