#include <termios.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <stdio.h>
#include <libgen.h>

#include "arg.h"
#include "keyword.h"
#include "path.h"

// takes in a char to see if its either '/', '.', or '~', aka a path
#define IS_A_PATH(c) (c == '/' ? 1 : c == '.' ? 1 : c == '~' ? 1 : 0)

// form an arg, allocating memory automatically
int parse_arg(arg_t *dest, arg_t src) {
	size_t src_i  = 0;
	size_t dest_i = 0;

	if (alloc_arg(dest, ARGV_ASZ) != 0) return 1;

	dest->len = 0;

	while (src_i < src.len) {
		dest->ptr[dest_i++] = src.ptr[src_i++];
		dest->len++;

		if (dest->len >= dest->asz) {
			if (alloc_arg(dest, ARGV_ASZ) != 0) return 1;
		}
	}

	return 0;
}

#define ARGV_CNT 1024

#define USER_ASZ 256

// edits an arg_t with content from fd, allocating memory automatically
// reads until a newline
int read_user_input(arg_t *dest, int fd) {
	if (alloc_arg(dest, USER_ASZ) != 0) return 1;
	size_t i = 0;
	char  ch = 0;

	while (read(fd, &ch, 1)) {
		dest->ptr[i++] = ch;
		dest->len++;

		if (dest->len > dest->asz) {
			if (alloc_arg(dest, ARGV_ASZ) != 0) return 1;
		}

		if (ch == '\n') break;
	}

	return 0;
}

// original terminal options
struct termios original_term = {0};

// return terminal to normal
void restore_term(void) {
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &original_term);
}

int main(int argc, char *argv[], char *envp[]) {
	(void)argc;(void)argv;(void)envp;

	// make $PATH
	refresh_path();

	/*
	// switch term options
	// get original options
	if (tcgetattr(STDIN_FILENO, &original_term) != 0) {
		fprintf(stderr, "%s: failed to get original terminal configuration: %s\n",
			basename(argv[0]), strerror(errno));
		return 1;
	}
	atexit(restore_term);

	// make new
	struct termios new_term = original_term;
	// non canonical and no echo
	new_term.c_cflag &= ~(ICANON | ECHO);
	new_term.c_cc[VMIN]  = 1;
	new_term.c_cc[VTIME] = 0;
	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &original_term) != 0) {
		fprintf(stderr, "%s: failed to set new terminal configuration: %s\n",
			basename(argv[0]), strerror(errno));
		return 1;
	}

	sleep(1);
	return 0;
	*/

	int status = 0;

	// main loop
	while (1) {
		if (status) {
			printf("(%d)> ", status % 255);
		} else {
			printf("> ");
		}

		status = 0;

		fflush(stdout);

		arg_t input = {0};
		if (read_user_input(&input, STDIN_FILENO)) return 1;

		if (arg_empty(input)) {
			free(input.ptr);
			putchar('\n');
			continue;
		}

		arg_t child_argv[ARGV_CNT] = {0};
		arg_t                 work = input;
		int			    child_argc = 0;

		while (work.len && child_argc < ARGV_CNT) {
			work = skip_whitespace(work);
			if (arg_empty(work)) break;

			// limits size to whitespace
			arg_t cmd_arg = trunc_to_white(work);

			if (parse_arg(&child_argv[child_argc], cmd_arg) != 0) return 1;

			child_argc++;
			work = goto_whitespace(work);
		}

		// turn into regular argv
		char *passed_argv[child_argc + 1];
		memset(passed_argv, '\0', sizeof(passed_argv));

		for (int i = 0; i < child_argc; i++) {
			passed_argv[i] = child_argv[i].ptr;
		}

		// try keywords
		int key = find_keyword(passed_argv[0]);
		if (key != NOT_A_KEYWORD) {
			switch (key) {
				case EXIT_KEYWORD_N: {
					exit_keyword();
					break;
				}

				case WHICH_KEYWORD_N: {
					which_keyword(child_argc, (const char **)passed_argv);
					break;
				}

				case ECHO_KEYWORD_N: {
					echo_keyword(child_argc, (const char **)passed_argv);
					break;
				}
			}

			goto end_looṕ;
		}

		char *bin_path = fetch_from_path(child_argv[0]);
		if (!bin_path) bin_path = passed_argv[0];

		pid_t child = fork();

		switch (child) {
			case -1: {
				return 1;
				break;
			}

			case 0: {
				execve(bin_path, passed_argv, envp);

				fprintf(stderr, "%s: couldn't execute \"%s\": %s\n",
				basename(argv[0]), bin_path, strerror(errno));
	   			fflush(stderr);
				return 0;

				break;
			}

			default: {
				while (waitpid(child, &status, WNOHANG) != -1) {};
				break;
			}
		}

		end_looṕ: {
			// free-em
			for (size_t i = 0; i < ARGV_CNT; i++) {
				if (!child_argv[i].ptr || !child_argv[i].asz) break;

				free(child_argv[i].ptr);
			}
		}
	}

	free_path();

	return 0;
}
