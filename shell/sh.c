#include "defs.h"
#include "types.h"
#include "readline.h"
#include "runcmd.h"

char prompt[PRMTLEN] = { 0 };

struct alternative_stack {
	/* data */
};


static void
sigchild_handler()
{
	pid_t pid;
	int status;

	pid = waitpid(0, &status, WNOHANG);

	if (pid > 0) {
		char buf[BUFLEN] = { 0 };

		snprintf(buf,
		         sizeof buf,
		         "=====> Process %d exited with status %d\n",
		         pid,
		         status);
		write(STDOUT_FILENO, buf, strlen(buf));
	}
}

// runs a shell command
static void
run_shell()
{
	char *cmd;

	while ((cmd = read_line(prompt)) != NULL)
		if (run_cmd(cmd) == EXIT_SHELL) {
			restore_default_signal_status(SIGCHLD);
			return;
		}
}

// initializes the shell
// with the "HOME" directory
static stack_t
init_shell()
{
	char buf[BUFLEN] = { 0 };
	char *home = getenv("HOME");

	if (chdir(home) < 0) {
		snprintf(buf, sizeof buf, "cannot cd to %s ", home);
		perror(buf);
	} else {
		snprintf(prompt, sizeof prompt, "(%s)", home);
	}

	setpgid(0, 0);

	struct sigaction sigchild_action = { .sa_handler = sigchild_handler,
		                             .sa_flags = SA_NOCLDSTOP |
		                                         SA_RESTART | SA_ONSTACK };

	stack_t alternative_stack = { .ss_sp = malloc(SIGSTKSZ),
		                      .ss_size = SIGSTKSZ,
		                      .ss_flags = 0 };

	if (alternative_stack.ss_sp == NULL) {
		exit(-1);
	}

	if (sigaltstack(&alternative_stack, 0) < 0) {
		free(alternative_stack.ss_sp);
		exit(-1);
	};
	if (sigaction(SIGCHLD, &sigchild_action, NULL) < 0) {
		free(alternative_stack.ss_sp);
		exit(-1);
	}

	return alternative_stack;
}

int
main(void)
{
	stack_t s = init_shell();

	run_shell();

	free(s.ss_sp);
	return 0;
}
