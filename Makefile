sshell: sshell.o
	gcc -o2 -Wall -Wextra -Werror sshell.o -o sshell

sshell.o: sshell.c
	gcc -o2 -Wall -Wextra -Werror -c -o sshell.o sshell.c

clean:
	rm -rf sshell.o sshell	