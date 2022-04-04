#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>

void create_pipe(int pipedf[], char *err_msg);
void write_in_pipe(int pipe, void *buf, size_t count, char *err_msg);
void read_from_pipe(int pipe, void *buf, size_t count, char *err_msg);

int get_validated_number(char *argv[]);
void calculate_primes(int parent_pipe, long int max);

void err_pipe_msg_iter(char err[], long int prime);
void err_read_msg_iter(char err[], long int prime);
void err_write_msg_iter(char err[], long int prime);

void
create_pipe(int pipedf[], char *err_msg)
{
	if (pipe(pipedf) < 0) {
		perror(err_msg);
		exit(-1);
	}
}

void
write_in_pipe(int pipe, void *buf, size_t count, char *err_msg)
{
	if (write(pipe, buf, count) < 0) {
		printf("%s\n", err_msg);
		exit(-1);
	}
}

void
read_from_pipe(int pipe, void *buf, size_t count, char *err_msg)
{
	if (read(pipe, buf, count) < 0) {
		printf("%s\n", err_msg);
		exit(-1);
	}
}

int
get_validated_number(char *argv[])
{
	char *endptr, *str = argv[1];
	errno = 0;
	long ret = strtol(str, &endptr, 10);

	if (str == endptr || '\0' != *endptr) {
		fprintf(stderr, "%s no es un numero decimal \n", str);
		exit(-1);
	}

	return ret;
}

void
err_pipe_msg_iter(char err[], long int prime)
{
	sprintf(err, "Error al crear el pipe principal en la iteracion del primo %li\n", prime);
}

void
err_read_msg_iter(char err[], long int prime)
{
	sprintf(err, "Error de lectura en la iteracion del primo %li", prime);
}

void
err_write_msg_iter(char err[], long int prime)
{
	sprintf(err, "Error de escritura en la iteracion del primo %li", prime);
}


void
calculate_primes(int recv_from_parent_pipe, long int max)
{
	long int current_num;
	int send_to_child_pipe = -1;
	// int process_pid = getpid();

	read_from_pipe(recv_from_parent_pipe,
	               &current_num,
	               sizeof(current_num),
	               "Error de lectura");

	long int current_prime = current_num;
	printf("primo %li\n", current_prime);

	char err_pipe_msg[80];
	// sprintf(err_pipe_msg, "Error al crear el pipe principal en la iteracion del primo %li\n", current_prime);
	char err_read_msg[80];
	// sprintf(err_read_msg, "Error de lectura en la iteracion del primo %li", current_prime);
	char err_write_msg[80];
	// sprintf(err_write_msg, "Error de escritura en la iteracion del primo %li", current_prime);

	int child_pipe[2];
	int child_pipe_created = 0;

	while ((2 <= current_num) && (current_num <= max)) {
		if ((current_num % current_prime) != 0) {
			if (!child_pipe_created) {
				err_pipe_msg_iter(err_pipe_msg, current_prime);
				create_pipe(child_pipe, err_pipe_msg);
				// printf("A) Proceso %d: Se abren el pipe de
				// lectura fd: %d y escritura fd: %d\n",
				// getpid(), child_pipe[0], child_pipe[1]);

				// printf("FORKEE\n");
				int process_pid = fork();

				if (process_pid < 0) {
					perror("Error en fork\n");
					exit(-1);
				}

				if (process_pid == 0) {
					/* child process*/

					current_prime = current_num;
					printf("primo %li\n", current_prime);
					close(child_pipe[1]);
					// printf("B) Proceso %d: Se cierra el fd: %d\n", getpid(), child_pipe[1]);
					close(recv_from_parent_pipe);
					// printf("C) Proceso %d: Se cierra el
					// fd: %d\n", getpid(), recv_from_parent_pipe);
					recv_from_parent_pipe = child_pipe[0];

				} else {
					/* parent process */

					close(child_pipe[0]);
					// printf("D) Proceso %d: Se cierra el fd: %d\n", getpid(), child_pipe[0]);
					close(send_to_child_pipe);
					// printf("E) Proceso %d: Se cierra el fd: %d\n", getpid(), send_to_child_pipe);
					send_to_child_pipe = child_pipe[1];
					child_pipe_created = 1;

					err_write_msg_iter(err_write_msg,
					                   current_prime);
					write_in_pipe(send_to_child_pipe,
					              &current_num,
					              sizeof(current_num),
					              err_write_msg);
				}
			} else {
				err_write_msg_iter(err_write_msg, current_prime);
				write_in_pipe(send_to_child_pipe,
				              &current_num,
				              sizeof(current_num),
				              err_write_msg);
			}

		} /* if current_buf is multiple of current_prime, then it is ignored */

		err_read_msg_iter(err_read_msg, current_prime);
		read_from_pipe(recv_from_parent_pipe,
		               &current_num,
		               sizeof(current_num),
		               err_read_msg);
	}

	if (child_pipe_created) {
		// printf("Proceso %d: ========== SANGU ==========\n", getpid());
		err_write_msg_iter(err_write_msg, current_prime);
		write_in_pipe(send_to_child_pipe,
		              &current_num,
		              sizeof(current_num),
		              err_write_msg);
		// printf("Proceso %d: ========== CHITO ==========\n", getpid());
	}

	close(recv_from_parent_pipe);
	close(send_to_child_pipe);
	// printf("F) Proceso %d: Se cierra el fd: %d\n", getpid(), recv_from_parent_pipe);

	/*
	if (process_pid == 0) {
	        close(recv_from_parent_pipe);
	        // printf("F) Proceso %d: Se cierra el fd: %d\n", getpid(),
	recv_from_parent_pipe); } else { close(send_to_child_pipe);
	        // printf("G) Proceso %d: Se cierra el fd: %d\n", getpid(),
	send_to_child_pipe);
	}
	*/
	wait(NULL);
}

int
main(int argc, char *argv[])
{
	if (argc != 2) {
		fprintf(stderr, "Numero equivocado de argumentos: %d\n", argc - 1);
		exit(-1);
	}

	long int num = get_validated_number(argv);

	// printf("Numero ingresado: %li\n", num);

	if (num < 2) {
		printf("No existen numeros primos\n");
		exit(-1);
	}

	int first_pipe[2];
	create_pipe(first_pipe, "Error al crear el pipe principal");
	// printf("H) Proceso %d: Se abren el pipe de lectura fd: %d y escritura
	// fd: %d\n", getpid(), first_pipe[0], first_pipe[1]);

	// printf("FORKEE\n");
	int process_pid = fork();

	if (process_pid == 0) {
		/* child process */

		close(first_pipe[1]);
		// printf("I) Proceso %d: Se cierra el fd: %d\n", getpid(), first_pipe[1]);
		calculate_primes(first_pipe[0], num);
		close(first_pipe[0]);
		// printf("J) Proceso %d: Se cierra el fd: %d\n", getpid(), first_pipe[0]);
	} else {
		/* parent process */

		close(first_pipe[0]);
		// printf("K) Proceso %d: Se cierra el fd: %d\n", getpid(), first_pipe[0]);
		for (long int i = 2; i <= num; i++) {
			write_in_pipe(
			        first_pipe[1],
			        &i,
			        sizeof(i),
			        "Error de escritura en el proceso principal");
		}
		int stop = 1;
		write_in_pipe(first_pipe[1],
		              &stop,
		              sizeof(stop),
		              "Error de escritura en el proceso principal");
		close(first_pipe[1]);
		// printf("L) Proceso %d: Se cierra el fd: %d\n", getpid(), first_pipe[1]);
		wait(NULL);
	}

	exit(0);
}
