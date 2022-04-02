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
	int process_pid;

	create_pipe(parent_to_child_pipedf, "Error en pipe de padre a hijo\n");
	create_pipe(child_to_parent_pipedf, "Error en pipe de hijo a padre\n");

	process_pid = fork();

	int recv_pair_buf[2];
	int recv_ran_buf;

	if (process_pid < 0) {
		perror("Error en fork\n");
		exit(-1);
	}

	if (process_pid == 0) {
		/* child process */

		/* close unused pipes for child*/
		close(parent_to_child_pipedf[1]);
		close(child_to_parent_pipedf[0]);

		read_from_pipe(parent_to_child_pipedf[0],
		               &recv_pair_buf,
		               sizeof(recv_pair_buf));
		printf("	- primer pipe me devuelve: [%d, %d]\n",
		       recv_pair_buf[0],
		       recv_pair_buf[1]);

		recv_pair_buf[0] += 3;
		recv_pair_buf[1] += 3;
		write_in_pipe(child_to_parent_pipedf[1],
		              &recv_pair_buf,
		              sizeof(recv_pair_buf));

		read_from_pipe(parent_to_child_pipedf[0],
		               &recv_ran_buf,
		               sizeof(recv_ran_buf));
	} else {
		/* parent process */

		/* close unused pipes for parent*/
		close(child_to_parent_pipedf[1]);
		close(parent_to_child_pipedf[0]);

		printf("Hola, soy PID %d:\n", getpid());

		int msg[2] = { 3, 4 };
		write_in_pipe(parent_to_child_pipedf[1], &msg, sizeof(msg));

		read_from_pipe(child_to_parent_pipedf[0],
		               &recv_pair_buf,
		               sizeof(recv_pair_buf));
		printf("	- segundo pipe me devuelve: [%d, %d]\n",
		       recv_pair_buf[0],
		       recv_pair_buf[1]);
	}

	printf("Donde fork me devuelve %d:\n", process_pid);
	printf("	- getpid me devuelve: %d\n", getpid());
	printf("	- getppid me devuelve: %d\n", getppid());

	if (process_pid == 0) {
		/* child process */

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

		int ran = random();
		printf("	- random me devuelve: %d\n", ran);

		write_in_pipe(parent_to_child_pipedf[1], &ran, sizeof(ran));
		printf("	- envío valor %d a traves de fd=%d\n",
		       ran,
		       parent_to_child_pipedf[1]);

		read_from_pipe(child_to_parent_pipedf[0],
		               &recv_ran_buf,
		               sizeof(recv_ran_buf));
		printf("Hola, de nuevo PID %d:\n", getpid());
		printf("	- recibí valor %d vía fd=%d\n",
		       ran,
		       child_to_parent_pipedf[0]);

		close(child_to_parent_pipedf[0]);
		close(parent_to_child_pipedf[1]);
		//wait(NULL);
	}

	fclose(stdin);
	fclose(stdout);
	fclose(stderr);

	exit(0);
}