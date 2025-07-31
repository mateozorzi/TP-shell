#include "exec.h"

// sets "key" with the key part of "arg"
// and null-terminates it
//
// Example:
//  - KEY=value
//  arg = ['K', 'E', 'Y', '=', 'v', 'a', 'l', 'u', 'e', '\0']
//  key = "KEY"
//
static void
get_environ_key(char *arg, char *key)
{
	int i;
	for (i = 0; arg[i] != '='; i++)
		key[i] = arg[i];

	key[i] = END_STRING;
}

// sets "value" with the value part of "arg"
// and null-terminates it
// "idx" should be the index in "arg" where "=" char
// resides
//
// Example:
//  - KEY=value
//  arg = ['K', 'E', 'Y', '=', 'v', 'a', 'l', 'u', 'e', '\0']
//  value = "value"
//
static void
get_environ_value(char *arg, char *value, int idx)
{
	size_t i, j;
	for (i = (idx + 1), j = 0; i < strlen(arg); i++, j++)
		value[j] = arg[i];

	value[j] = END_STRING;
}

// sets the environment variables received
// in the command line
//
// Hints:
// - use 'block_contains()' to
// 	get the index where the '=' is
// - 'get_environ_*()' can be useful here
static void
set_environ_vars(char **eargv, int eargc)
{
	for (int i = 0; i < eargc; i++) {
		char key[BUFLEN];
		char value[BUFLEN];
		int idx = block_contains(eargv[i], '=');

		if (idx < 0) {
			continue;
		}

		get_environ_key(eargv[i], key);
		get_environ_value(eargv[i], value, idx);

		setenv(key, value, BUFLEN);
	}
}

// opens the file in which the stdin/stdout/stderr
// flow will be redirected, and returns
// the file descriptor
//
// Find out what permissions it needs.
// Does it have to be closed after the execve(2) call?
//
// Hints:
// - if O_CREAT is used, add S_IWUSR and S_IRUSR
// 	to make it a readable normal file
static int
open_redir_fd(char *file, int flags)
{
	int fd;
	if (flags & O_CREAT) {
		fd = open(file, flags | O_CLOEXEC, S_IRUSR | S_IWUSR);
	}

	else {
		fd = open(file, flags | O_CLOEXEC);
	}

	if (fd < 0) {
		perror("open");
		return -1;
	}

	return fd;
}

// executes a command - does not return
//
// Hint:
// - check how the 'cmd' structs are defined
// 	in types.h
// - casting could be a good option
void
exec_cmd(struct cmd *cmd)
{
	// To be used in the different cases
	struct execcmd *e;
	struct backcmd *b;
	struct execcmd *r;
	struct pipecmd *p;

	switch (cmd->type) {
	case EXEC:
		// spawns a command
		//
		e = (struct execcmd *) cmd;

		set_environ_vars(e->eargv, e->eargc);
		if (execvp(e->argv[0], e->argv) < 0) {
			perror("Erros execvp");
			_exit(-1);
		}

		break;

	case BACK: {
		b = (struct backcmd *) cmd;
		exec_cmd(b->c);

		break;
	}

	case REDIR: {
		// changes the input/output/stderr flow
		//
		// To check if a redirection has to be performed
		// verify if file name's length (in the execcmd struct)
		// is greater than zero
		r = (struct execcmd *) cmd;

		if (strlen(r->in_file) > 0) {
			int fd = open_redir_fd(r->in_file, O_RDONLY);
			if (fd >= 0) {
				dup2(fd, STDIN_FILENO);
				close(fd);
			} else {
				perror("open_redir_fd");
				_exit(-1);
			}
		}

		if (strlen(r->out_file) > 0) {
			int fd = open_redir_fd(r->out_file,
			                       O_CREAT | O_WRONLY | O_TRUNC);
			if (fd >= 0) {
				dup2(fd, STDOUT_FILENO);
				close(fd);
			} else {
				perror("open_redir_fd");
				_exit(-1);
			}
		}

		if (strlen(r->err_file) > 0) {
			// if not equal to "&1"
			if (strcmp(r->err_file, "&1") != 0) {
				int fd = open_redir_fd(r->err_file,
				                       O_CREAT | O_WRONLY |
				                               O_TRUNC);
				if (fd >= 0) {
					dup2(fd, STDERR_FILENO);
					close(fd);
				} else {
					perror("open_redir_fd");
					_exit(-1);
				}
			} else {
				dup2(STDOUT_FILENO, STDERR_FILENO);
			}
		}


		int err = execvp(r->argv[0], r->argv);

		if (err == -1) {
			perror("Error al ejecutar comando");
			_exit(-1);
			break;
		}

		break;
	}

	case PIPE: {
		// pipes two commands
		p = (struct pipecmd *) cmd;
		int fds[2];

		if (pipe(fds) < 0) {
			perror("Error al crear el pipe");
			_exit(-1);
		}

		int f_izq = fork();

		if (f_izq < 0) {
			perror("Error de fork");
			close(fds[READ]);
			close(fds[WRITE]);
			_exit(-1);
		} else if (f_izq == 0) {
			// hijo izquierdo
			close(fds[READ]);
			dup2(fds[WRITE], STDOUT_FILENO);
			close(fds[WRITE]);
			exec_cmd(p->leftcmd);
			_exit(-1);
		}

		int f_der = fork();

		if (f_der < 0) {
			perror("Error de fork");
			close(fds[READ]);
			close(fds[WRITE]);
			_exit(-1);
		} else if (f_der == 0) {
			// hijo derecho
			close(fds[WRITE]);
			dup2(fds[READ], STDIN_FILENO);
			close(fds[READ]);
			exec_cmd(p->rightcmd);
			_exit(-1);
		}

		close(fds[READ]);
		close(fds[WRITE]);
		waitpid(f_izq, NULL, 0);
		waitpid(f_der, NULL, 0);

		free_command(parsed_pipe);
		exit(0);

		break;
	}
	}
}
