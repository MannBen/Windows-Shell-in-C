#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>

#define CMDLINE_MAX 512

//cd /home/cs150jp/public/p1/ 
int main(void)
{
        char cmd[CMDLINE_MAX], cmdcopy[CMDLINE_MAX];
        char workdir[CMDLINE_MAX];
        
        int toFileBool = 0;

        while (1) {
                char *nl, *n2;
                char *arguments[16];
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
                strncpy(cmdcopy, cmd, CMDLINE_MAX);// save copy of orig cmd in before real editing starts
                /* find > characters */
                n2 = strchr(cmd, '>');
                if(n2){
                        *n2 = ' ';
                        toFileBool = 1;
                }

                //printf("cmd raw: %s\n",cmd);

                /* Builtin command */
                // this is the command for exit
                if (!strcmp(cmd, "exit")) {
                        fprintf(stderr, "Bye...\n");
                        break;
                } 
                // the is the command for pwd 
                if (!strcmp(cmd, "pwd")) {
                        printf("CURRENT DIR: %s\n", getcwd(workdir, CMDLINE_MAX));
                } 


                //add initial token
                char *token = strtok(cmd, " ");
                arguments[0] = token;

                //first attemp
                // check for output redirectation 
                // if(strchr(cmd, '>')) {
                //         token = strtok(NULL, ">");
                //         printf("token: %s", token);
                //         arguments[1] = token;
                //     // write(STDOUT_FILENO(cmd, arguments[0].length()))   
                // }

                //jillian attempt
                // while (token !=NULL){
                //         char* carrot = strchr(token, '>');
                //         if(carrot != NULL){
                //                 token = strtok(NULL, ">");
                //                 printf("token1: %s\n",token);
                //         }
                //         else{
                //                 token = strtok(NULL, " ");
                //                 printf("token2: %s\n",token);
                //         }
                //         arguments[inc] = token;
                //         inc++; 
                // }
                
                //third and kinda working attemp but bad code/scale into p5/on
                // while(token != NULL){
                //         //make object 
                //         n2 = strchr(cmd, '>');
                //         token = strtok(NULL, " ");
                //         //printf("token1: %s\n",token);
                //         //n2 = strchr(arguments[(inc-1)], '>');
                //         if (n2) { //really bad code
                //                 *n2 = ' ';
                //                 toFileFD = open(token, O_WRONLY | O_CREAT, 0644);
                //                 savedSTDO = dup(STDIN_FILENO);
                //                 dup2(toFileFD, STDOUT_FILENO);
                //                 close(toFileFD);
                //                 token = NULL; //file target, hardcoded NULL when passed to ecec for now but probably needs to change
                //                  //printf("> found %s\n",token);
                //         }
                //         arguments[inc] = token;// add token to arguments
                //         inc++; 
                // }

                while(token != NULL) {
                        //make object 
                      //  printf("token1: %s\t%d\n",token, inc);
                        token = strtok(NULL, " ");
                        arguments[inc] = token;// add token to arguments
                        inc++; 
                       // printf("token2: %s\t%d\n",token, inc);
                }
                

                if (!strcmp(arguments[0], "cd")) {
                       chdir(arguments[1]);
                       continue;
                } 
                

                pid_t pid;
                pid = fork();
                int status;


                if (pid == 0) {
                        // child
                          //printf("args: %s\n",arguments[(inc - 2)]);
                        if (toFileBool){
                              //  printf("args: %s\n",arguments[(inc - 2)]);
                                toFileFD = open(arguments[(inc - 2)], O_WRONLY | O_CREAT, 0644);
                                savedSTDO = dup(STDOUT_FILENO);
                                dup2(toFileFD, STDOUT_FILENO);
                                close(toFileFD);
                        }

                        // printf("cmd raw: %s\n",cmd);
                        char *args[] = { arguments[0], arguments[1], arguments[2], arguments[3], 
                                         arguments[4], arguments[5], arguments[6], arguments[7], 
                                         arguments[8], arguments[9], arguments[10], arguments[11], 
                                         arguments[12], arguments[13], arguments[14], arguments[15]};
                        execvp(arguments[0], args); 
                        perror("execvp");
                        exit(1);
                } else if (pid > 0) {
                        // parent
                        waitpid(pid, &status, 0);
                        //return to STDO
                        dup2(savedSTDO, toFileFD);
                        close(savedSTDO);
                        toFileBool == 0;
                }else {
                        perror("fork");
                        exit(1);
                }


                // show status 
                fprintf(stderr, "+ completed '%s' [%d]\n",
                        cmdcopy, WEXITSTATUS(status));
        }

        return EXIT_SUCCESS;
}
