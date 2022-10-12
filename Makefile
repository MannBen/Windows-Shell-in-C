sshell: sshell.o
	gcc -Wall -Wextra -Werror sshell.o -o sshell

sshell.o: sshell.c
	gcc -Wall -Wextra -Werror -c -o sshell.o sshell.c

clean:
	rm -rf sshell.o sshell	