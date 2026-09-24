#include <limits.h>
#include <stddef.h>
#include <termios.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <stdio.h>
#include <libgen.h>

#include "parse.h"
#include "envp.h"
#include "exec.h"
#include "path.h"
#include "arg.h"

// TODO: globbin' (globbing, if you don't like having fun...)
// TODO: user input has to be non-canonical and respond immediately
// to key presses, to process everything
// TODO: ignore ctrl-c and other commands
// TODO: &&, ||, <, >, |, etc
//
// future TODO: logic (while, if, for, etc), make it C-like

// edits an arg_t with content from fd, allocating memory automatically
// reads until a newline
int read_fd_line(arg_t *dest, int fd) {
	size_t i = 0;
	char  ch = 0;

	// here, the terminal should be raw and echo-less, and
	// input would be handled dynamically, such as opening
	// a prompt for missing quotes or unfinished backslashes

	ssize_t r = 0;
	bool nl = false;
	while (1) {
		r = read(fd, &ch, 1);
		if (r == -1)  return 1;
		else if (r == 0) break;

		if ((dest->len + 1) > dest->asz) {
			if (alloc_arg(dest, ARGV_ASZ) != 0) return 1;
		}

		if (ch == '\n') nl = true;

		dest->ptr[i++] = nl == true ? '\0' : ch;
		dest->len++;

		if (nl == true) break;
	}

	return 0;
}

int main(int argc, char *argv[], char *envp[]) {
	(void)argc;(void)argv;(void)envp;

	// make the envp passed down to children
	if (make_child_envp((const char **)envp)) return 1;
	// make $PATH
	//
	// hash map is made here too
	refresh_path();

	// stuff
	global_argv0 = argv[0];

	int           status = 0;
	arg_t          input = {0};
	arg_arr_t child_argv = {0};

	// main loop
	while (1) {
		if (status) {
			printf("(%d)> ", status % 255);
		} else {
			printf("> ");
		}

		status = 0;
		fflush(stdout);

		if (read_fd_line(&input, STDIN_FILENO)) return 1;

		if (arg_empty(input)) {
			// put newline if needed
			if (!input.ptr) putchar('\n');
			else if (!strchr(input.ptr, '\n')) putchar('\n');

			zero_arg(&input);
			continue;
		}

		arg_t remade = {0};
		if (remake_arg(&remade, input) != 0) return 1;

		int child_argc = 0;
		if (make_child_argv(remade, &child_argv, &child_argc, ARGV_ARR_ASZ) != 0) return 1;

		if (!child_argc) goto loop_end;

		// turn into regular argv
		char **passed_argv = calloc(sizeof(passed_argv[0]), child_argc + 1);
		if (!passed_argv) return 1;

		for (int i = 0; i < child_argc; i++) {
			passed_argv[i] = child_argv.ptr[i].ptr;
		}

		status = execute(child_argc, passed_argv, child_envp.dp, 0);

		loop_end: {
			zero_arg_arr(&child_argv);
			zero_arg(&input);
			free_arg(&remade);
			// passed_argv doesn't get freed

			// yes, i know this isn't the right thing...
			if (!isatty(STDIN_FILENO)) break;
		}
	}

	free_arg_arr(&child_argv);
	free_dblp(&child_envp);
	free_arg(&input);
	free_path();

	return 0;
}
