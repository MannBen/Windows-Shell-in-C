sshell: sshell.o
	gcc sshell.o -o sshell

sshell.o: sshell.c
	gcc -Wall -Wextra -Werror -c sshell.c

clean:
	rm -rf sshell.o sshell