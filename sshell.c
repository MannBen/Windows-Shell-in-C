#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define CMDLINE_MAX 512

//cd /home/cs150jp/public/p1/ 
int main(void)
{
        char cmd[CMDLINE_MAX];

        while (1) {
                char *nl;
                int retval;

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
                if (!strcmp(cmd, "exit")) {
                        fprintf(stderr, "Bye...\n");
                        break;
                } 

                /* Regular command */
                // retval = system(cmd);

                pid_t pid;
                /* date - in
                output: 
                ./date
                Return status vaue for 'date': 0
                */

                /*
                        char *args[] = { argv[counter], NULL };
                        execvp(args[0], args);


                        char *cmd = "date";
                        char *args[] = { cmd, "ECS150", NULL};
                */
                pid = fork();
                if (pid == 0) {
                        // child
                        char *args[] = { cmd, NULL };
                        retval = execvp(cmd, args); //WEXITSTATUS?
                        perror("execvp");
                        exit(1);
                } else if (pid > 0) {
                        // parent
                        int status;
                        waitpid(pid, &status, 0);
                        //printf("Child returned %d/n", WEXITSTATUS(status));
                }else {
                        perror("fork");
                        exit(1);
                }

                fprintf(stderr, "+ completed '%s' [%d]\n",
                        cmd, retval);
        }

        return EXIT_SUCCESS;
}
