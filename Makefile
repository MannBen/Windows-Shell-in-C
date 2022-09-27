sshell: sshell.o
	g++ -std=c++0x -Wall -Wextra -Werror sshell.o -o sshell

sshell.o: sshell.c
	g++ -c sshell.c

clean:
	rm -rf sshell.o 	sshell