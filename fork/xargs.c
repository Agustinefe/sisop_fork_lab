#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <errno.h>

#ifndef NARGS
#define NARGS 4
#endif

void execute_command(char* argv[], size_t argv_size);
void clean_newline(char *arg, ssize_t read_bytes);
void free_args(char* argv[], size_t argv_size);
void manage_args(char* command);

void execute_command(char* argv[], size_t argv_size) {

	argv[argv_size] = NULL;

	int pid = fork();

	if (pid < 0) {
		perror("Ha ocurrido un problema con el fork\n");
		exit(-1);
	}

	if (pid == 0) {
		int err = execvp(argv[0], argv);
		if (err == errno) perror("No se ha podido ejecutar el comando\n");
	} 

	wait(NULL);
}

void clean_newline(char *arg, ssize_t read_bytes) {
	for (ssize_t i = 0; i < read_bytes; i++) {
		if (arg[i] == '\n') {
			arg[i] = '\0';
		}
	}
}

void free_args(char* argv[], size_t argv_size) {
	
	for (size_t i = 1; i < argv_size; i++) {
		free(argv[i]);
		argv[i] = 0;
	}

}

void manage_args(char* command) {

	char *argv[NARGS+2] = {0};
	argv[0] = command;
	argv[NARGS+1] = NULL;
	char *buf = NULL;
	ssize_t read_bytes = 0;
	int argv_pos = 1;
	size_t len = 0;

	while((read_bytes = getline(&buf, &len, stdin)) != -1) {

		clean_newline(buf, read_bytes);
		argv[argv_pos] = buf;
		buf = NULL;
		argv_pos++;

		if(argv_pos >= NARGS + 1) {

			execute_command(argv, argv_pos);
			free_args(argv, argv_pos);
			argv_pos = 1;		

		}

	}

	execute_command(argv, argv_pos);
	free_args(argv, argv_pos);
	free(buf);

}


int
main(int argc, char *argv[])
{
	if (argc != 2){
		perror("La cantidad de parametros es incorrecta.\n");
		exit(-1);
	}

	manage_args(argv[1]);

	exit(0);
}