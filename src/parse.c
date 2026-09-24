#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "parse.h"
#include "alias.h"
#include "path.h"
#include "envp.h"
#include "arg.h"

// remake an arg (like input), doing the following:
//
// 1. replacing the immediate first argument with an alias
// 2. (to be added) globbing files
// 3. consuming args that are DEFS=something
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
			char *b = calloc(1, arg.len + 1);
			if (!b) return 1;
			memcpy(b, arg.ptr, arg.len);

			// make the two strings
			b[eq - arg.ptr] = '\0';
			char *label = b;
			char   *val = &b[eq - arg.ptr + 1];

			shell_set_env(label, val);
			if (!strcmp(label, "PATH")) refresh_path();

			free(b);
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

		/*
		case '[': {
			op = '[';
			cl = ']';
			break;
		}
		*/

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

		/*
		case '[': {
			content = expand_cmd(start);
			break;
		}
		*/

		default: {
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
	bool back = false;

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
				back = true;
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

			if (!back) {
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
			} else {
				back = false;
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

// parse an arg into an arg_arr_t
bool make_child_argv(const arg_t arg, arg_arr_t *child_argv, int *child_argc, size_t asz) {
	if (!child_argv || !child_argc) return 1;

	arg_t work = arg;

	while (work.len) {
		if ((size_t)(*child_argc + 1) >= (child_argv->asz / sizeof(arg_t))) {
			if (alloc_arg_arr(child_argv, asz) != 0) return 1;
		}

		work = skip_whitespace(work);

		// limits size to whitespace
		arg_t cmd_arg = cap_to_white(work);
		if (arg_empty(cmd_arg)) break;

		if (parse_arg(&child_argv->ptr[*child_argc], cmd_arg) != 0) return 1;

		// end early if fully empty stuff
		if (arg_empty(child_argv->ptr[*child_argc])) {
			goto skip;
		}

		(*child_argc)++;

		skip: {
			work = goto_whitespace(work);
		}
	}

	return 0;
}
