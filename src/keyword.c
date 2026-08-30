#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libgen.h>
#include <limits.h>
#include <errno.h>
#include <stdio.h>

#include "keyword.h"
#include "arg.h"
#include "path.h"
#include "alias.h"

#define KEY_SUCCESS     0
#define KEY_PARTIAL_ERR 1
#define KEY_FULL_ERR    2

// relates a string to a keyword
int find_keyword(const char *str) {
    if (!strcmp(str, EXIT_KEYWORD_S)) {
        return EXIT_KEYWORD_N;

    } else if (!strcmp(str, WHICH_KEYWORD_S)) {
    	return WHICH_KEYWORD_N;

    } else if (!strcmp(str, ECHO_KEYWORD_S)) {
    	return ECHO_KEYWORD_N;

    } else if (!strcmp(str, CD_KEYWORD_S)) {
    	return CD_KEYWORD_N;

    } else if (!strcmp(str, PWD_KEYWORD_S)) {
    	return PWD_KEYWORD_N;

    } else if (!strcmp(str, ENV_KEYWORD_S)) {
    	return ENV_KEYWORD_N;

    } else if (!strcmp(str, EXPORT_KEYWORD_S)) {
    	return EXPORT_KEYWORD_N;

    } else if (!strcmp(str, UNSET_KEYWORD_S)) {
    	return UNSET_KEYWORD_N;

    } else if (!strcmp(str, PATH_KEYWORD_S)) {
    	return PATH_KEYWORD_N;

    } else if (!strcmp(str, ALIAS_KEYWORD_S)) {
    	return ALIAS_KEYWORD_N;

    } else if (!strcmp(str, UNALIAS_KEYWORD_S)) {
    	return UNALIAS_KEYWORD_N;

    } else {
        return NOT_A_KEYWORD;
    }
}

int exit_keyword(void) {
    exit(0);
    return KEY_SUCCESS;
}

int which_keyword(const int argc, const char *argv[]) {
	if (argc < 2) {
		fprintf(stderr, "%s: too little arguments.\n", WHICH_KEYWORD_S);
		fflush(stderr);
		return KEY_FULL_ERR;
	}

	bool err = false;
	for (int i = 1; i < argc; i++) {
		char *resolved = fetch_from_path(make_arg(argv[i]));

		int key = find_keyword(argv[i]);
		if (key != NOT_A_KEYWORD) {
			printf("%s -> <built-in>\n", argv[i]);
			continue;
		}

		if (!resolved) {
			fprintf(stderr, "%s: unknown binary \"%s\".\n", WHICH_KEYWORD_S, argv[i]);
			err = true;

		} else {
			printf("%s -> %s\n", argv[i], resolved);
		}
	}

	fflush(stdout);
	if (err == true) fflush(stderr);
	return err ? KEY_PARTIAL_ERR : KEY_SUCCESS;
}

int echo_keyword(const int argc, const char *argv[]) {
	int i = 1;
	while (i < argc) {
		printf("%s", argv[i++]);
		if (i < argc) putchar(' ');
	}

	putchar('\n');
	fflush(stdout);

	return KEY_SUCCESS;
}

int cd_keyword(const int argc, const char *argv[]) {
	bool err = false;

	if (argc < 2) {
		// no args leads directly to home
		char *home = getenv("HOME");
		if (!home) {
			fprintf(stderr, "%s: failed to get home directory path.\n",
				CD_KEYWORD_S);
		}

		if (chdir(home) != 0) {
			fprintf(stderr, "%s: couldn't change to \"%s\": %s\n",
				CD_KEYWORD_S, argv[1], strerror(errno));
		}

		fflush(stderr);
		return KEY_FULL_ERR;

	} else if (argc > 2) {
		fprintf(stderr, "%s: excess arguments shall be ignored.\n", CD_KEYWORD_S);
	}

	if (chdir(argv[1]) != 0) {
		fprintf(stderr, "%s: couldn't change to \"%s\": %s\n",
			CD_KEYWORD_S, argv[1], strerror(errno));
		fflush(stderr);
		err = true;
	}

	return err ? KEY_PARTIAL_ERR : KEY_SUCCESS;
}

int pwd_keyword(void) {
	char path[PATH_MAX] = {0};
	getcwd(path, sizeof(path));
	printf("%.*s\n", (int)sizeof(path), path);
	fflush(stdout);
	return KEY_SUCCESS;
}

int env_keyword(const char *envp[]) {
	while (*envp) {
		printf("%s\n", *envp);
		envp++;
	}
	fflush(stdout);
	return KEY_SUCCESS;
}

int export_keyword(const int argc, const char *argv[]) {
	if (argc < 2) {
		fprintf(stderr, "%s: too little arguments.\n", EXPORT_KEYWORD_S);
		fflush(stderr);
		return KEY_FULL_ERR;
	}

	bool err = false;
	for (int i = 1; i < argc; i++) {
		arg_t arg = make_arg(argv[i]);

		char *eq = arg_chr(arg, '=');
		if (eq == arg.ptr) {
			fprintf(stderr, "%s: malformed request.\n", EXPORT_KEYWORD_S);
			err = true;
			continue;
		}

		if (eq) *eq = '\0';

		if (eq && !strcmp(argv[i], "PATH")) refresh_path();

		if (setenv(argv[i], eq ? eq + 1 : "", true) != 0) {
			fprintf(stderr, "%s: failed to set environment variable \"%s\": %s\n",
				EXPORT_KEYWORD_S, argv[i], strerror(errno));
			err = true;
			continue;
		}

		if (eq) *eq = '=';
	}

	if (err == true) fflush(stderr);

	return err ? KEY_PARTIAL_ERR : KEY_SUCCESS;
}

int unset_keyword(const int argc, const char *argv[]) {
	bool err = false;
	if (argc < 2) {
		fprintf(stderr, "%s: too little arguments.\n", UNSET_KEYWORD_S);
		fflush(stderr);
		return KEY_FULL_ERR;
	}

	for (int i = 1; i < argc; i++) {
		fprintf(stderr, "");
		if (unsetenv(argv[i]) != 0) {
			fprintf(stderr, "%s: failed to unset environment variable \"%s\": %s\n",
				EXPORT_KEYWORD_S, argv[i], strerror(errno));
			err = true;
			continue;
		}
	}

	if (err == true) fflush(stderr);
	return err ? KEY_PARTIAL_ERR : KEY_SUCCESS;
}

int path_keyword(void) {
	size_t i = 0;
	char *path;
	while ((path = reveal_path(&i))) {
		char *f = basename(path);
		printf("%.*s/\e[1m%s\e[0m\n", (int)(f - path) - 1, path, f);
	}
	fflush(stdout);
	return KEY_SUCCESS;
}

int alias_keyword(const int argc, const char *argv[]) {
	bool err = false;
	if (argc < 2) {
		fprintf(stderr, "%s: too little arguments.\n", ALIAS_KEYWORD_S);
		fflush(stderr);
		return KEY_FULL_ERR;
	}

	for (int i = 1; i < argc; i++) {
		arg_t arg = make_arg(argv[i]);

		// make sure there's an equal sign
		char *eq = arg_chr(arg, '=');
		if (!eq) {
			fprintf(stderr, "%s: malformed request.\n", ALIAS_KEYWORD_S);
			err = true;
			continue;
		}

		if (new_alias(cap_to_char(arg, '='), shift_arg(shift_arg_c(arg, '='), 1))) {
			fprintf(stderr, "%s: failed to make a new alias.\n", ALIAS_KEYWORD_S);
			err = true;
			continue;
		}
	}

	if (err == true) fflush(stderr);
	return err ? KEY_PARTIAL_ERR : KEY_SUCCESS;
}

int unalias_keyword(const int argc, const char **argv) {
	bool err = false;
	if (argc < 2) {
		fprintf(stderr, "%s: too little arguments.\n", UNALIAS_KEYWORD_S);
		fflush(stderr);
		return KEY_FULL_ERR;
	}

	for (int i = 1; i < argc; i++) {
		if (remove_alias(make_arg(argv[i])) != 0) {
			fprintf(stderr, "%s: failed to remove an alias.\n", UNALIAS_KEYWORD_S);
			err = true;
			continue;
		}
	}

	if (err == true) fflush(stderr);
	return err ? KEY_PARTIAL_ERR : KEY_SUCCESS;
}
