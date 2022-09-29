#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
//#include <sys/wait.h> 


#define CMDLINE_MAX 512

//cd /home/cs150jp/public/p1/ 
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


                //add initial token
                char *token = strtok(cmd, " ");
                arguments[0] = token;
                
   
                while(token != NULL){
                        //make object 
                        token = strtok(NULL, " ");
                        // add token to array 
                        arguments[inc] = token;
                        inc++; 
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
                }else {
                        perror("fork");
                        exit(1);
                }
                // show status 
                fprintf(stderr, "+ completed '%s' [%d]\n",
                        cmd, WEXITSTATUS(status));
        }

        return EXIT_SUCCESS;
}
