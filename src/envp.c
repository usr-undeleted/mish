#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "envp.h"
#include "arg.h"

#define ALLOC_SZ 32

dbl_ptr_t child_envp = {0};

// a bit of a deceiving name, but all it will hold
// are unexported environment variables
//
// new env vars will be put here, and only put on child_envp
// when exported
//
// all strings inside must be allocated individually
dbl_ptr_t shell_envp = {0};

// produce child_envp, making sure to add a null term
bool make_child_envp(const char **envp) {
	if (!envp) return 1;
	free_dblp(&child_envp);

	// both of the copies don't add a null term, intentionally

	// copy over everything from original envp
	while (*envp) {
		if (append_to_dblp(&child_envp, *(envp++), ALLOC_SZ) != 0) return 1;
	}

	// copy over everything from shell_envp (if it exists)
	if (shell_envp.dp) {
		size_t i = 0;
		while (shell_envp.dp[i] && i < shell_envp.icnt) {
			if (append_to_dblp(&child_envp, shell_envp.dp[i], ALLOC_SZ)) return 1;
			i++;
		}
	}

	// now's the null term
	if (append_to_dblp(&child_envp, NULL, ALLOC_SZ)) return 1;

	return 0;
}

// set a new (or overwrite) an env var to the shell envp
//
// val may be NULL for nothing, but an empty string would
// also work, i guess
bool shell_set_env(const char *label, const char *val) {
	if (!label) return 1;

	// strlen(label) + '=' + strlen(val) + '\0', basically
	size_t llen = strlen(label);
	size_t vlen = val ? strlen(val) : 0;
	size_t sz = llen + 2 + vlen;

	// allocate
	char *s = calloc(1, sz);
	if (!s) return 1;

	// make the string
	strncat(s, label, sz);
	s[llen] = '=';
	if (val) strncat(s, val, sz - llen - 1);

	bool exists = false;
	size_t i = find_dblp_entry_c(shell_envp, label, &exists, '=');
	if (exists == true) {
		// replace
		free(shell_envp.dp[i]);
		shell_envp.dp[i] = s;

	} else {
		// append new
		// will overwrite null term
		if (append_to_dblp(&shell_envp, s, ALLOC_SZ)) return 1;
		// re-add it
		if (append_to_dblp(&shell_envp, NULL, ALLOC_SZ)) return 1;
	}

	return 0;
}

// remove an environment variable from all contexts,
// for example, freeing the memory from shell_envp, and
// removing it from child_envp too
bool unset_env(const char *label) {
	// remove from child_envp first
	s_remove_dbpl_entry_c(&child_envp, label, '=');

	// remove from shell_envp now
	bool exists = false;
	size_t i = find_dblp_entry_c(shell_envp, label, &exists, '=');
	if (exists == false) return 0;

	char *p = shell_envp.dp[i];
	s_remove_dbpl_entry_c(&shell_envp, label, '=');
	free(p);

	return 0;
}

// move an env var from the shell context to the
// child context
//
// if val is already set on env:
// val field is NULL: copy it over
// val field is non-zero: set env and copy it over
bool export_env(const char *label, const char *val) {
	bool exists = false;
	size_t i = find_dblp_entry_c(shell_envp, label, &exists, '=');

	// if the val exists, remove it
	if (exists == true && val && unset_env(label) != 0) return 1;

	// make the env
	if (shell_set_env(label, val == NULL ? shell_get_env(label) : val) != 0) return 1;

	i = find_dblp_entry_c(shell_envp, label, &exists, '=');
	if (exists == false) return 1;

	bool c_exists = false;
	size_t ci = find_dblp_entry_c(child_envp, label, &c_exists, '=');

	if (c_exists == true) {
		// already exists, replace
		child_envp.dp[ci] = shell_envp.dp[i];

	} else {
		// append
		if (append_to_dblp(&child_envp, shell_envp.dp[i], ALLOC_SZ) != 0) return 1;
		if (append_to_dblp(&child_envp, NULL, ALLOC_SZ) != 0) return 1;
	}

	return 0;
}

// get an env, used by the shell
char *shell_get_env(const char *label) {
	char *p = dblp_get_ptr_c(shell_envp, label, '=');
	if (!p) p = dblp_get_ptr_c(child_envp, label, '=');

	char *ret = p ? strchr(p, '=') + 1 : NULL;
	return ret;
}
