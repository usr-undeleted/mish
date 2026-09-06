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

#include "alias.h"
#include "arg.h"
#include "envp.h"
#include "exec.h"
#include "path.h"

#define ARGV_ARR_ASZ 8

// TODO: globbin' (globbing, if you don't like having fun...)
// TODO: user input has to be non-canonical and respond immediately
// to key presses, to process everything
// TODO: ignore ctrl-c and other commands
// TODO: command expansion
// TODO: &&, ||, <, >, |, etc
//
// future TODO: logic (while, if, for, etc), make it C-like

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

// expands a src (at src_i) into an env var, or cmd expansion, etc
//
// if it fails to expand, return 1
//
// src is expected to be at dollar sign
bool expand(arg_t *dest, arg_t src, size_t *src_i) {
	if (src.ptr[*src_i] != '$') return 1;

	// skip dollar sign
	++(*src_i);

	// starts after dollar sign
	arg_t arg = {
		.ptr = src.ptr + *src_i,
		.len = src.len - *src_i,
		.asz = src.asz - *src_i,
	};
	char op = '\0';
	char cl = '\0';

	// find the proper stuff
	switch (src.ptr[*src_i]) {
		case '(': {
			op = '(';
			cl = ')';
			break;
		}

		// command expansion goes here, for example

		default: {
			--(*src_i);
			return 1;
		}
	}

	// find the closing thingie
	char *close = find_closer(arg, op, cl);
	if (!close) return 1;

	// encapsulates the whole $(THING)
	arg.len = close - arg.ptr + 1;

	// used to find env var
	char *start = arg.ptr + 1;
	*close = '\0';

	// make sure to skip everything from the source
	*src_i += strlen(start) + 2;
	char *content = NULL;
	switch (op) {
		case '(': {
			// env vars
			content = shell_get_env(start);
			break;
		}

		// command expansion goes here, for example

		default: {
			content = NULL;
			break;
		}
	}

	// wasnt found
	if (!content) {
		return 0;
	};

	// append to dest
	// increase size if needed
	size_t cont_len = strlen(content);
	if ((dest->len + cont_len) >= dest->asz) {
		if (alloc_arg(dest, MAX(cont_len, ARGV_ASZ)) != 0) return 1;
	}

	dest->len += snprintf(dest->ptr + dest->len,
		dest->asz - dest->len,
		"%s", content);

	*close = cl;

	return 0;
}

// form an arg, allocating memory automatically
int parse_arg(arg_t *dest, arg_t src) {
	if (!dest) return 1;
	free_arg(dest);
	size_t src_i  = 0;
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
				if (expand(dest, src, &src_i) != 0) goto copy_memory;

				continue;
			}

			// home dir on start
			case '~': {
				if (quote_type == QUOTE_SIN || dest->len != 0) goto copy_memory;

				char *env = shell_get_env("HOME");
				if (!env) goto copy_memory;

				size_t env_len = strlen(env);
				if ((dest->len + env_len) >= dest->asz) {
					if (alloc_arg(dest, MAX(env_len, ARGV_ASZ)) != 0) return 1;
				}

				dest->len += snprintf(dest->ptr + dest->len,
					dest->asz - dest->len,
					"%.*s", (int)env_len, env);

				++src_i;
				continue;
			}

			case '\\': {
				if (quote_type != NO_QUOTES) goto copy_memory;
				++src_i;

				goto copy_memory;
			}

			default: {
				goto copy_memory;
				break;
			}
		}

		copy_memory: {
			if (src_i >= src.len) break;

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

// edits an arg_t with content from fd, allocating memory automatically
// reads until a newline
int read_fd_line(arg_t *dest, int fd) {
	size_t i = 0;
	char  ch = 0;

	// here, the terminal should be raw and echo-less, and
	// input would be handled dynamically, such as opening
	// a prompt for missing quotes or unfinished backslashes

	ssize_t r = 0;
	while (1) {
		r = read(fd, &ch, 1);
		if (r == -1)  return 1;
		else if (r == 0) break;

		if ((dest->len + 1) > dest->asz) {
			if (alloc_arg(dest, ARGV_ASZ) != 0) return 1;
		}

		dest->ptr[i++] = ch;
		dest->len++;

		if (ch == '\n') break;
	}

	return 0;
}

// remake an arg (like input), doing the following:
// 1. replacing the immediate first argument with an alias
// 2. (to be added) globbing files
int remake_arg(arg_t *dest, const arg_t src) {
	if (!dest) return 1;
	dest->len = 0;
	size_t src_i = 0;

	bool aliased = false;
	bool not_def = false;

	while (src_i < src.len) {
		// realloc if needed
		if (dest->len >= dest->asz) {
			if (alloc_arg(dest, ARGV_ASZ) != 0) return 1;
		}

		arg_t arg = cap_to_white(shift_arg(src, src_i));

		// arg is something like DEFINE=SOMETHING
		if (not_def == false && arg_is_def(arg)) {
			char *eq = arg_chr(arg, '=');
			if (!eq) {
				not_def = true;
				goto skip;
			}

			// make the two strings
			// hopefully one could find a way to not use the heap here
			size_t llen = eq - arg.ptr;
			char *label = calloc(1, llen + 1);
			size_t vlen = arg.len - ((eq + 1) - arg.ptr);
			char   *val = calloc(1, vlen + 1);
			if (!val || !label) return 1;
			// copy over
			memcpy(label, arg.ptr, llen);
			memcpy(val, eq + 1, vlen);

			if (shell_set_env(label, val) != 0) return 1;

			free(val);
			free(label);
			skip: {
				idx_to_white(&src_i, src);
				continue;
			}

		} else not_def = true;

		// aliases
		if (aliased == false && !EMPTY_C(src.ptr[src_i])) {
			// make the alias
			arg_t alias = find_alias(arg, NULL);

			// put it in the dest
			if ((dest->len + alias.len) >= dest->asz) {
				if (alloc_arg(dest, MAX(ARGV_ASZ, alias.len))) return 1;
			}

			dest->len += snprintf(dest->ptr + dest->len, dest->asz - dest->len,
				"%.*s", (int)alias.len, alias.ptr);

			idx_to_white(&src_i, src);
			aliased = true;
			continue;
		}

		dest->ptr[dest->len++] = src.ptr[src_i++];
	}

	return 0;
}

int main(int argc, char *argv[], char *envp[]) {
	(void)argc;(void)argv;(void)envp;

	// make $PATH
	refresh_path();
	// make the envp passed down to children
	if (make_child_envp((const char **)envp)) return 1;

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

		arg_t       work = remade;
		int   child_argc = 0;

		while (work.len) {
			if ((size_t)(child_argc + 1) >= (child_argv.asz / sizeof(arg_t))) {
				if (alloc_arg_arr(&child_argv, ARGV_ARR_ASZ) != 0) return 1;
			}

			work = skip_whitespace(work);

			// limits size to whitespace
			arg_t cmd_arg = cap_to_white(work);
			if (arg_empty(cmd_arg)) break;

			if (parse_arg(&child_argv.ptr[child_argc], cmd_arg) != 0) return 1;

			// end early if fully empty stuff
			if (arg_empty(child_argv.ptr[child_argc])) {
				goto skip;
			}

			child_argc++;

			skip: {
				work = goto_whitespace(work);
			}
		}

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
