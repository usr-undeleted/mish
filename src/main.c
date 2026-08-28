#include <ctype.h>
#include <stddef.h>
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

#define NO_QUOTES 0
#define QUOTE_DBL 1
#define QUOTE_SIN 2

// get the quote type of a char
#define QUOTE_T(c) (c == '\"' ? QUOTE_DBL : c == '\'' ? QUOTE_SIN : NO_QUOTES)
// check if a char is empty
#define EMPTY_C(c) (isspace(c) ? 1 : iscntrl(c) ? 1 : 0)
// takes in a char to see if its either '/', '.', or '~', aka a path
#define IS_A_PATH(c) (c == '/' ? 1 : c == '.' ? 1 : c == '~' ? 1 : 0)
// get the biggest value
#define MAX(x, y) (x > y ? x : y)

// find the equivalent closer for the opener
// returns null on failure to find the closer, or when the
// pointer provided isn't the open char
char *find_closer(arg_t arg, const char open, const char close) {
	if (*arg.ptr != open) return NULL;

	size_t depth = 0;
	size_t i     = 0;

	while (i < arg.len) {
		if (arg.ptr[i] == open) {
			depth++;
		} else if (arg.ptr[i] == close) {
			depth--;
		}

		if (!depth) return &arg.ptr[i];

		i++;
	}

	return NULL;
}

// form an arg, allocating memory automatically
int parse_arg(arg_t *dest, arg_t src) {
	size_t src_i  = 0;

	if (alloc_arg(dest, ARGV_ASZ) != 0) return 1;

	dest->len = 0;

	char quote_type = NO_QUOTES;

	while (src_i < src.len) {
		// realloc if needed
		if (dest->len >= dest->asz) {
			if (alloc_arg(dest, ARGV_ASZ) != 0) return 1;
		}

		switch (src.ptr[src_i]) {
			// stuff like env vars
			case '$': {
				if (quote_type == QUOTE_SIN) goto copy_memory;

				// skip dollar sign
				src_i++;
				switch (src.ptr[src_i]) {
					// env var expansion
					case '(': {
						arg_t arg = {
							.ptr = src.ptr + src_i,
							.len = src.len - src_i,
							.asz = src.asz - src_i,
						};

						char *close = find_closer(arg, '(', ')');

						if (!close) {
							// since we failed, move back and copy
							--src_i;
							goto copy_memory;
						}

						// encapsulates the whole $(ENV)
						arg.len = close - arg.ptr + 1;

						char *start = arg.ptr + 1;
						*close = '\0';

						// make sure to skip everything from the source
						src_i += strlen(start) + 2;
						char *env = getenv(start);
						if (!env) {
							break;
						};

						// append to dest
						// increase size if needed
						size_t env_len = strlen(env);
						if ((dest->len + env_len) >= dest->asz) {
							if (alloc_arg(dest, MAX(env_len, ARGV_ASZ)) != 0) return 1;
						}

						dest->len += snprintf(dest->ptr + dest->len,
							dest->asz - dest->len,
							"%s", env);
						*close = ')';

						break;
					}

					default: {
						--src_i;
						goto copy_memory;
					}
				}

				continue;
			}

			// home dir on start
			case '~': {
				if (quote_type == QUOTE_SIN) goto copy_memory;

				if (dest->len == 0) {
					char *env = getenv("HOME");
					if (!env) goto copy_memory;

					size_t env_len = strlen(env);
					if ((dest->len + env_len) >= dest->asz) {
						if (alloc_arg(dest, MAX(env_len, ARGV_ASZ)) != 0) return 1;
					}

					dest->len += snprintf(dest->ptr + dest->len,
						dest->asz - dest->len,
						"%.*s", (int)env_len, env);

					++src_i;

				} else goto copy_memory;

				continue;
			}

			default: {
				goto copy_memory;

				break;
			}
		}

		copy_memory: {
			char char_quote = QUOTE_T(src.ptr[src_i]);

			if (char_quote) {
				if (!quote_type) {
					// quotes haven't been set
					quote_type = char_quote;
					++src_i;

				} else {
					if (quote_type == char_quote) {
						quote_type = 0;
						++src_i;
					}

				}

				continue;
			}

			dest->ptr[dest->len++] = src.ptr[src_i++];
		}
	}

	//null term
	if (dest->len > dest->asz) {
		if (alloc_arg(dest, 1) != 0) return 1;
	}
	dest->ptr[dest->len] = '\0';

	return 0;
}

#define ARGV_ARR_ASZ 8

// edits an arg_t with content from fd, allocating memory automatically
// reads until a newline
int read_user_input(arg_t *dest, int fd) {
	size_t i = 0;
	char  ch = 0;

	while (read(fd, &ch, 1)) {
		if ((dest->len + 1) > dest->asz) {
			if (alloc_arg(dest, ARGV_ASZ) != 0) return 1;
		}

		dest->ptr[i++] = ch;
		dest->len++;

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
	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &new_term) != 0) {
		fprintf(stderr, "%s: failed to set new terminal configuration: %s\n",
			basename(argv[0]), strerror(errno));
		return 1;
	}
	*/

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

		if (read_user_input(&input, STDIN_FILENO)) return 1;

		if (arg_empty(input)) {
			// put newline if needed
			if (!input.ptr) putchar('\n');
			else if (!strchr(input.ptr, '\n')) putchar('\n');

			zero_arg(&input);
			continue;
		}

		arg_t             work = input;
		int			child_argc = 0;

		while (work.len) {
			if ((size_t)(child_argc + 1) >= (child_argv.asz / sizeof(arg_t))) {
				if (alloc_arg_arr(&child_argv, ARGV_ARR_ASZ) != 0) return 1;
			}

			work = skip_whitespace(work);
			if (arg_empty(work)) break;

			// limits size to whitespace
			arg_t cmd_arg = cap_to_white(work);

			if (parse_arg(&child_argv.ptr[child_argc], cmd_arg) != 0) return 1;

			// end early if the arg is empty
			if (arg_empty(child_argv.ptr[child_argc]) ||
				EMPTY_C(*child_argv.ptr[child_argc].ptr)) {

					free_arg(&child_argv.ptr[child_argc]);
					break;
			}

			child_argc++;
			work = goto_whitespace(work);
		}

		// turn into regular argv
		char **passed_argv = calloc(sizeof(passed_argv[0]), child_argc + 1);
		if (!passed_argv) return 1;
		memset(passed_argv, '\0', sizeof(passed_argv[0]) * (child_argc + 1));

		for (int i = 0; i < child_argc; i++) {
			passed_argv[i] = child_argv.ptr[i].ptr;
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

				case CD_KEYWORD_N: {
					cd_keyword(child_argc, (const char **)passed_argv);
					break;
				}

				case PWD_KEYWORD_N: {
					pwd_keyword();
					break;
				}

				case ENV_KEYWORD_N: {
					env_keyword((const char **)envp);
					break;
				}

				case EXPORT_KEYWORD_N: {
					export_keyword(child_argc, (const char **)passed_argv);
					break;
				}

				case UNSET_KEYWORD_N: {
					unset_keyword(child_argc, (const char **)passed_argv);
					break;
				}

				case PATH_KEYWORD_N: {
					path_keyword();
					break;
				}

				default: {
					fprintf(stderr, "%s: unknown keyword %d (internal)\n",
						basename(argv[0]), key);
					fflush(stderr);
					break;
				}
			}

			goto end_loop;
		}

		char *bin_path = fetch_from_path(child_argv.ptr[0]);
		if (!bin_path) {
			if (!IS_A_PATH(*child_argv.ptr[0].ptr)) {
				fprintf(stderr, "%s: couldn't execute \"%s\": Unknown binary\n",
					basename(argv[0]), child_argv.ptr[0].ptr);
				fflush(stderr);
				goto end_loop;

			} else {
				bin_path = passed_argv[0];
			}
		}

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

		end_loop: {
			zero_arg_arr(&child_argv);
			zero_arg(&input);
			if (!isatty(STDIN_FILENO)) break;
		}
	}

	free_arg_arr(&child_argv);
	free_arg(&input);
	free_path();

	return 0;
}
