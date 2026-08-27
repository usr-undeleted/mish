#include <termios.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <sys/wait.h>
#include <stdio.h>
#include <libgen.h>

// *_alloc_sz macros to define how much
// to allocate for at once
#define ARGV_ASZ 16

// string should include newline
typedef struct {
	char  *ptr;
	size_t len;
	// allocation size
	size_t asz;

} arg_t;

// make a new arg, going to the nearest whitespace
arg_t goto_whitespace(const arg_t arg) {
	arg_t ret = arg;

	while (!isspace(*ret.ptr) && ret.len) {
		ret.ptr++;
		ret.len--;
	}

	return ret;
}

// make a new arg, skipping whitespace
arg_t skip_whitespace(const arg_t arg) {
	arg_t ret = arg;

	while (isspace(*ret.ptr) && ret.len) {
		ret.ptr++;
		ret.len--;
	}

	return ret;
}

// decrease the args length till it encapsulates
// everything not in whitespace
arg_t trunc_to_white(const arg_t arg) {
	arg_t  ret = arg;
	size_t i = 0;

	while (!isspace(ret.ptr[i]) && i < ret.len) {
		i++;
	}

	ret.len = i;

	return ret;
}

// (re)allocates a pointer for an arg
int alloc_arg(arg_t *arg, const size_t sz) {
	// if it doesn't exist
	if (!arg->ptr) {
		arg->ptr = calloc(sizeof(char), sz);
		if (!arg->ptr) return 1;

		arg->len = 0;
		arg->asz = sz;

		return 0;
	}

	// else, add to allocation
	arg->ptr = realloc(arg->ptr, arg->asz + sz);
	if (!arg->ptr) return 1;

	memset(arg->ptr + arg->asz, '\0', sz);
	arg->asz += sz;

	return 0;
}

// form an arg, allocating memory automatically
int parse_arg(arg_t *dest, arg_t src) {
	size_t src_i  = 0;
	size_t dest_i = 0;

	if (alloc_arg(dest, ARGV_ASZ) != 0) return 1;

	dest->len = 0;

	while (src_i < src.len) {
		dest->ptr[dest_i++] = src.ptr[src_i++];
		dest->len++;

		if (dest->len > dest->asz) {
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

// see if an arg is empty
// return 1 on yes
int arg_empty(arg_t arg) {
	for (size_t i = 0; i < arg.len; i++) {
		if (!isspace(arg.ptr[i])) return 0;
	}

	return 1;
}

// original terminal options
struct termios original_term = {0};

// return terminal to normal
void restore_term(void) {
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &original_term);
}

int main(int argc, char *argv[], char *envp[]) {
	(void)argc;(void)argv;(void)envp;
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
	// main loop
	while (1) {
		printf("> ");
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
		int             child_argc = 0;

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

		pid_t child = fork();

		switch (child) {
			case -1: {
				return 1;
				break;
			}

			case 0: {
				execve(passed_argv[0], passed_argv, envp);
				fprintf(stderr, "%s: couldn't execute \"%s\": %s\n",
					basename(argv[0]), passed_argv[0], strerror(errno));
				fflush(stderr);
				return 0;
				break;
			}

			default: {
				int status;
				waitpid(child, &status, 0);
				break;
			}
		}

		// free-em
		for (size_t i = 0; i < ARGV_CNT; i++) {
			if (!child_argv[i].ptr || !child_argv[i].asz) break;

			free(child_argv[i].ptr);
		}
	}


	return 0;
}
