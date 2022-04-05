#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/times.h>
#include <sys/wait.h>

void create_pipe(int pipedf[], char *err_msg);
void write_in_pipe(int pipe, void *buf, size_t count);
void read_from_pipe(int pipe, void *buf, size_t count);

void
create_pipe(int pipedf[], char *err_msg)
{
	if (pipe(pipedf) < 0) {
		perror(err_msg);
		exit(-1);
	}
}

void
write_in_pipe(int pipe, void *buf, size_t count)
{
	if (write(pipe, buf, count) < 0) {
		perror("Error de escritura\n");
		exit(-1);
	}
}

void
read_from_pipe(int pipe, void *buf, size_t count)
{
	if (read(pipe, buf, count) < 0) {
		perror("Error de lectura\n");
		exit(-1);
	}
}

int
main(void)
{
	srandom(times(NULL));
	int parent_to_child_pipedf[2]; /* pipe[0]: child reads, pipe[1]: parent writes */
	int child_to_parent_pipedf[2]; /* pipe[0]: parent reads, pipe[1:] child writes */

	printf("Hola, soy PID %d:\n", getpid());

	create_pipe(parent_to_child_pipedf, "Error en pipe de padre a hijo\n");
	create_pipe(child_to_parent_pipedf, "Error en pipe de hijo a padre\n");

	printf("	- primer pipe me devuelve: [%d, %d]\n",
		parent_to_child_pipedf[0],
		parent_to_child_pipedf[1]);

	printf("	- segundo pipe me devuelve: [%d, %d]\n",
		child_to_parent_pipedf[0],
		child_to_parent_pipedf[1]);

	int process_pid;

	if ((process_pid = fork()) < 0) {
		perror("Error en fork\n");
		exit(-1);
	}

	printf("Donde fork me devuelve %d:\n", process_pid);
	printf("	- getpid me devuelve: %d\n", getpid());
	printf("	- getppid me devuelve: %d\n", getppid());

	int send_ran_buf;
	int recv_ran_buf;

	if (process_pid == 0) {
		/* child process */

		read_from_pipe(parent_to_child_pipedf[0], &recv_ran_buf, sizeof(recv_ran_buf));

		printf("	- recibo valor %d vía fd=%d\n",
		       recv_ran_buf,
		       parent_to_child_pipedf[0]);

		write_in_pipe(child_to_parent_pipedf[1],
		              &recv_ran_buf,
		              sizeof(recv_ran_buf));
		printf("	- reenvío valor en fd=%d y termino\n",
		       child_to_parent_pipedf[1]);

		close(parent_to_child_pipedf[0]);
		close(child_to_parent_pipedf[1]);
	} else {
		/* parent process */

		send_ran_buf = random();
		printf("	- random me devuelve: %d\n", send_ran_buf);

		write_in_pipe(parent_to_child_pipedf[1], &send_ran_buf, sizeof(send_ran_buf));
		printf("	- envío valor %d a traves de fd=%d\n",
		       send_ran_buf,
		       parent_to_child_pipedf[1]);

		read_from_pipe(child_to_parent_pipedf[0],
		               &recv_ran_buf,
		               sizeof(recv_ran_buf));
		printf("Hola, de nuevo PID %d:\n", getpid());
		printf("	- recibí valor %d vía fd=%d\n",
		       recv_ran_buf,
		       child_to_parent_pipedf[0]);

		close(child_to_parent_pipedf[0]);
		close(parent_to_child_pipedf[1]);
		wait(NULL);
	}

	exit(0);
}