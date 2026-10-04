#include <asm-generic/errno-base.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <libgen.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>

#include "parse.h"
#include "alias.h"
#include "exec.h"
#include "path.h"
#include "envp.h"
#include "arg.h"

// remake an arg (like input), doing the following:
//
// 1. replacing the immediate first argument with an alias
// 2. (to be added) globbing files
// 3. consuming args that are DEFS=something
bool remake_arg(arg_t *dest, const arg_t src) {
	if (!dest) return 1;
	dest->len = 0;
	size_t src_i = 0;

	bool aliased = false;

	while (src_i < src.len) {
		// realloc if needed
		if (dest->len >= dest->asz) {
			if (alloc_arg(dest, ARGV_ASZ) != 0) return 1;
		}

		arg_t arg = cap_to_white(shift_arg(src, src_i));

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

#define PIPE_READ  0
#define PIPE_WRITE 1

// expand a command
//
// memory is allocated, make sure to free it
char *expand_cmd(const char *src) {
	(void)src;

	int fd[2] = {0};
	if (pipe(fd) != 0) return NULL;
	if (fcntl(fd[PIPE_READ], F_SETFL, O_NONBLOCK) == -1) return NULL;

	pid_t child = fork();
	switch (child) {
		case -1: return NULL;

		case 0: {
			dup2(fd[PIPE_WRITE], STDOUT_FILENO);

			arg_t remade = {0};
			if (remake_arg(&remade, make_arg(src)) != 0) exit(0);
			if (arg_empty(remade)) exit(0);

			arg_arr_t child_argv = {0};

			int child_argc = 0;
			if (make_child_argv(remade, &child_argv, &child_argc, ARGV_ARR_ASZ) != 0) exit(0);
			if (!child_argc) exit(0);

			// turn into regular argv
			char **passed_argv = calloc(sizeof(passed_argv[0]), child_argc + 1);
			if (!passed_argv) exit(0);

			for (int i = 0; i < child_argc; i++) {
				passed_argv[i] = child_argv.ptr[i].ptr;
			}

			execute(child_argc, passed_argv, child_envp.dp, NO_FORK);

			exit(0);
			break;
		}

		default: {
			int status;
			while (waitpid(child, &status, WNOHANG) != -1) {};

			arg_t buf = {0}; // returned
			char   ch = 0;
			while (read(fd[PIPE_READ], &ch, 1) && errno != EAGAIN) {
				if (buf.len >= buf.asz) {
					if (alloc_arg(&buf, ARGV_ASZ) != 0) return NULL;
				}

				buf.ptr[buf.len++] = ch;
			}

			// replace final whitespace with null
			if (buf.len && EMPTY_C(buf.ptr[buf.len - 1])) buf.ptr[buf.len - 1] = '\0';
			// null term
			else if (buf.len && (buf.ptr[buf.len - 1] != '\0' || EMPTY_C(buf.ptr[buf.len - 1]))) {
				if (alloc_arg(&buf, 1) != 0) return NULL;
				buf.ptr[buf.len] = '\0';
			}

			close(fd[PIPE_READ]);
			close(fd[PIPE_WRITE]);
			return buf.ptr;
			break;
		}
	}

	return NULL;
}

// expands a src (at src_i) into an env var, or cmd expansion, etc
//
// if it fails to expand, return 1
//
// src is expected to be at dollar sign
bool expand(arg_t *dest, const arg_t src, size_t *src_i) {
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

		case '[': {
			op = '[';
			cl = ']';
			break;
		}

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

	// make the string for comparisons

	arg_t comp = {0};
	if (parse_arg(&comp, make_arg(start), 0) != 0) return 1;

	switch (op) {
		case '(': {
			// env vars
			content = shell_get_env(comp.ptr);
			break;
		}

		case '[': {
			// command expansion
			content = expand_cmd(comp.ptr);
			break;
		}
	}

	free_arg(&comp);

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
	if (op == '[') free(content);

	return 0;
}

// form an arg, allocating memory automatically
//
// return 1 on errors
//
// f field adds extra functionality (that acts independent of quote):
// - turning escape characters into their counterparts
bool parse_arg(arg_t *dest, arg_t src, bool f) {
	if (!dest) return 1;
	free_arg(dest);
	size_t src_i  = 0;
	dest->len = 0;

	char quote_type = NO_QUOTES;
	bool back = false;
	bool  esc = false;

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
				if (f) esc = true;
				if (quote_type != NO_QUOTES) goto copy_memory;
				back = true;
				++src_i;

				goto copy_memory;
			}

			default: {
				goto copy_memory;
				break;
			}
		}

		copy_memory:
		if (src_i >= src.len) break;

		if (esc) {
			esc = false;

			switch (src.ptr[src_i]) {
				case 'e': {
					dest->ptr[dest->len++] = '\x1b';
					++src_i;
					continue;
					break;
				}
			}
		}

		if (!back && determine_quote(src.ptr[src_i], &quote_type)) {
			++src_i;
			continue;

		} else {
			back = false;
		}

		dest->ptr[dest->len++] = src.ptr[src_i++];
	}

	// null term
	if (dest->len > dest->asz || !dest->ptr) {
		if (alloc_arg(dest, 1) != 0) return 1;
	}
	dest->ptr[dest->len] = '\0';

	return 0;
}

// parse an arg into an arg_arr_t
bool make_child_argv(const arg_t arg, arg_arr_t *child_argv, int *child_argc, size_t asz) {
	if (!child_argv || !child_argc) return 1;

	arg_t    work = arg;
	bool def_able = true;

	while (work.len) {
		if ((size_t)(*child_argc + 1) >= (child_argv->asz / sizeof(arg_t))) {
			if (alloc_arg_arr(child_argv, asz) != 0) return 1;
		}

		work = skip_whitespace(work);

		// limits size to whitespace
		arg_t cmd_arg = cap_to_white(work);
		if (arg_empty(cmd_arg)) break;

		if (parse_arg(&child_argv->ptr[*child_argc], cmd_arg, 0) != 0) return 1;

		// end early if fully empty stuff
		if (arg_empty(child_argv->ptr[*child_argc])) {
			goto skip;
		}

		// definitions get skipped and used up
		if (arg_is_def(child_argv->ptr[*child_argc]) && def_able) {
			char *eq = arg_chr(child_argv->ptr[*child_argc], '=');
			if (!eq) goto skip;

			// make the two strings
			char *b = calloc(1, child_argv->ptr[*child_argc].len + 1);
			if (!b) return 1;
			memcpy(b, child_argv->ptr[*child_argc].ptr, child_argv->ptr[*child_argc].len);

			// make the two strings
			b[eq - child_argv->ptr[*child_argc].ptr] = '\0';
			char *label = b;
			char   *val = &b[eq - child_argv->ptr[*child_argc].ptr + 1];

			shell_set_env(label, val);
			if (!strcmp(label, "PATH")) refresh_path();

			goto skip;
		} else def_able = false;

		(*child_argc)++;

		skip: {
			work = goto_whitespace(work);
		}
	}

	return 0;
}
