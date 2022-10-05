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

int argHandler(char* arguments[], char []); 
void createProcess(char *[]);

void createProcess(char *cleanedArguments[]){
                pid_t pid;
                pid = fork();
                int status;


                if (pid == 0) {
                        // child
                          //printf("args: %s\n",arguments[(inc - 2)]);
                        // if (toFileBool){
                        //       //  printf("args: %s\n",arguments[(inc - 2)]);
                        //         toFileFD = open(arguments[(inc - 2)], O_WRONLY | O_CREAT, 0644);
                        //         savedSTDO = dup(STDOUT_FILENO);
                        //         dup2(toFileFD, STDOUT_FILENO);
                        //         close(toFileFD);
                        // }

                        // printf("cmd raw: %s\n",cmd);
                         //cleaned arguments
                        execvp(cleanedArguments[0], cleanedArguments); 
                        perror("execvp");
                        exit(1);
                } else if (pid > 0) {
                        // parent
                        waitpid(pid, &status, 0);
                        //return to STDO
                        // dup2(savedSTDO, toFileFD);
                        // close(savedSTDO);
                        // toFileBool = 0;
                }else {
                        perror("fork");
                        exit(1);
                }

                     fprintf(stderr, "+ completed '%s' [%d]\n",
                        cleanedArguments[0], WEXITSTATUS(status)); //change to full command string later
}
void createProcessWithOutputRedirection(char *cleanedArguments[], char* outputFile){
                pid_t pid;
                pid = fork();
                int status, toFileFD;


                if (pid == 0) {
                        // child
                          //printf("args: %s\n",arguments[(inc - 2)]);
                   
                        toFileFD = open(outputFile, O_WRONLY | O_CREAT, 0644);
                        dup2(toFileFD, STDOUT_FILENO);
                        close(toFileFD);
                        execvp(cleanedArguments[0], cleanedArguments); 
                        perror("execvp");
                        exit(1);
                } else if (pid > 0) {
                        // parent
                        waitpid(pid, &status, 0);
                        //return to STDO
                        // dup2(savedSTDO, toFileFD);
                        // close(savedSTDO);
                        // toFileBool = 0;
                }else {
                        perror("fork");
                        exit(1);
                }

                     fprintf(stderr, "+ completed '%s' [%d]\n",
                        cleanedArguments[0], WEXITSTATUS(status)); //change to full command string later
}


int argHandler(char* arguments[], char workdir[]) {
    int currPos = 0;
    //char workdir[512];
    char *cleanedArguments[16];
    char *outRedirect, *pipe, *inRedirect;
    bool hasChar = false;

    //n2 = strchr(cmd, '\n');
    //n3 = strchr(cmd, '\n');
    // >
    // <
    // |

    //echo hello world > test | cat test
    // echo echo test | 11

    while(arguments[currPos] != NULL){
        // initialize special characters
        outRedirect = strchr(arguments[currPos], '>');
        pipe = strchr(arguments[currPos], '|');
        inRedirect = strchr(arguments[currPos], '<');
        //if not < > >&  | add to cleanargs
        // If not a > < or | then go in if statement
        if(outRedirect == 0 && pipe == 0 && inRedirect == 0){
            cleanedArguments[currPos] = arguments[currPos];
            printf("args: %s\t%d\n", cleanedArguments[currPos], currPos);
            currPos++;
        }
        else{
            if(outRedirect != 0) {
                // this will handle the > output redirection
                printf("In carrot statement\n");
                hasChar = true;
                cleanedArguments[currPos] = NULL;
                createProcessWithOutputRedirection(cleanedArguments, arguments[currPos+1]);
                currPos += 2;
                break; // this will then break out of the else and continue going through argumetns

            } else if(inRedirect != 0) {
                printf("In < Statement\n");
                //then handle input redirection <
                break;
            }
//            else{
//                cleanedArguments[currPos] = NULL;
//                createProcess(cleanedArguments);
//            }
            break;
        }
        // check for pipe
        //checkpipe() which will be like below
        // can be up to 3 pipe signs on the same command line
         if  (pipe != 0) {
                 // process 1 | process 2 | process 3
                 // handle pipe after checking if no > or <
                 printf("In | Statement\n");
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
    // this will create a process if string doesnt have a > < or |
    else if (!hasChar) {
        cleanedArguments[currPos] = NULL;
        createProcess(cleanedArguments);
    }
    return 0;
}


int main(void)
{
        char cmd[CMDLINE_MAX];
        char workdir[CMDLINE_MAX];
        char *arguments[16];
        int toFileBool = 0;

        while (1) {
                char *nl, *n2;
                int inc = 1, toFileFD, savedSTDO;

                /* Print prompt */
                printf("sshell@ucd$ ");
                fflush(stdout);

                /* Get command line */
                fgets(cmd, CMDLINE_MAX, stdin);
                //printf("cmd raw: %s\n",cmd);

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
               
                /* find > characters */
                // n2 = strchr(cmd, '>');
                // if(n2){
                //         *n2 = ' ';
                //         toFileBool = 1;
                // }
                //raw tokens
                //add initial token
                char *token = strtok(cmd, " ");
                arguments[0] = token;

                while(token != NULL) {
                        //make object 
                        //printf("token1: %s\t%d\n",token, inc);
                        token = strtok(NULL, " ");
                        arguments[inc] = token;// add token to arguments
                        inc++; 
                       // printf("token2: %s\t%d\n",token, inc);
                }

                argHandler(arguments, workdir);

                // pid_t pid;
                // pid = fork();
                // int status;


                // if (pid == 0) {
                //         // child
                //           //printf("args: %s\n",arguments[(inc - 2)]);
                //         if (toFileBool){
                //               //  printf("args: %s\n",arguments[(inc - 2)]);
                //                 toFileFD = open(arguments[(inc - 2)], O_WRONLY | O_CREAT, 0644);
                //                 savedSTDO = dup(STDOUT_FILENO);
                //                 dup2(toFileFD, STDOUT_FILENO);
                //                 close(toFileFD);
                //         }

                //         // printf("cmd raw: %s\n",cmd);
                //         char *args[] = { arguments[0], arguments[1], arguments[2], arguments[3], 
                //                          arguments[4], arguments[5], arguments[6], arguments[7], 
                //                          arguments[8], arguments[9], arguments[10], arguments[11], 
                //                          arguments[12], arguments[13], arguments[14], arguments[15]}; //cleaned arguments
                //         execvp(arguments[0], args); 
                //         perror("execvp");
                //         exit(1);
                // } else if (pid > 0) {
                //         // parent
                //         waitpid(pid, &status, 0);
                //         //return to STDO
                //         dup2(savedSTDO, toFileFD);
                //         close(savedSTDO);
                //         toFileBool = 0;
                // }else {
                //         perror("fork");
                //         exit(1);
                // }


                // show status 
                // fprintf(stderr, "+ completed '%s' [%d]\n",
                //         cmdcopy, WEXITSTATUS(status));
        }

        return EXIT_SUCCESS;
}
