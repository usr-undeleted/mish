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
#include "envp.h"

#define KEY_SUCCESS     0
#define KEY_PARTIAL_ERR 1
#define KEY_FULL_ERR    2

// a flag array will be made of this
typedef struct {
	// the flag
	char   ch;
	// the string
	char *str;
	// the pointer the bool
	bool *val;

} flag_t;

// return a value greater or equal to zero on success, else, a negative
// number is returned
//
// aka uhm, returns an idx into flags
ssize_t ch_is_flag(const char ch, const flag_t *flags, const size_t fc) {
	for (size_t i = 0; i < fc; i++) {
		if (ch == flags[i].ch) return i;
	}

	return -1;
}

// see the flags for an arg
//
// deals with both single char
//
// fc == flag count
//
// return 0 on success (or not a flag arg), 1 on unknown flag
bool flag(const char *str, const flag_t *flags, const size_t fc) {
	if (!str || !flags) return 1;
	bool match = false;

	if (str[0] == '-' && str[1] == '-') {
		// loop trough full flags
		for (size_t i = 0; i < fc; i++) {
			// find a match
			if (!strcmp(str + 2, flags[i].str)) {
				match = true;
				if (flags[i].val) *flags[i].val = true;
				break;
			}
		}

	} else if (str[0] == '-') {
		// single flags
		size_t i = 1;
		size_t x;

		while (str[i]) {
			if ((x = ch_is_flag(str[i], flags, fc)) >= 0) {
				match = true;
				*flags[x].val = true;

			} else return 1;
			++i;
		}

	} else return 0;

	return match == true ? 0 : 1;
}

// get a value from a label inside an envp double pointer
char *envp_get(char *envp[], const char *label) {
	bool exists = false;
	size_t i = find_dblp_entry_c(make_dblp(envp), label, &exists, '=');
	if (exists == false) return NULL;

	char *eq = strchr(envp[i], '=');
	if (!eq) return NULL;

	char *ret = eq + 1;

	return ret;
}

// relates a string to a keyword
int find_keyword(char *str) {
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

    } else if (!strcmp(str, RETURN_KEYWORD_S)) {
    	return RETURN_KEYWORD_N;

    } else {
        return NOT_A_KEYWORD;
    }
}

int exit_keyword(void) {
    exit(0);
    return KEY_SUCCESS;
}

#define WHICH_KEY_PATH_ONLY_S "path-only"
#define WHICH_KEY_PATH_ONLY_C 'c'
#define      WHICH_KEY_HELP_S "help"
#define      WHICH_KEY_HELP_C 'h'

int which_keyword(int argc, char *argv[]) {
	if (argc < 2) {
		fprintf(stderr, "%s: not 'nuff arguments.\n", WHICH_KEYWORD_S);
		fflush(stderr);
		return KEY_FULL_ERR;
	}

	bool only_path = false;
	bool      help = false;

	flag_t flags[] = {
		// path flag
		{
			.ch  = WHICH_KEY_PATH_ONLY_C,
			.str = WHICH_KEY_PATH_ONLY_S,
			.val = &only_path,
		},

		// help flag
		{
			.ch  = WHICH_KEY_HELP_C,
			.str = WHICH_KEY_HELP_S,
			.val = &help,
		}
	};

	size_t fc = sizeof(flags) / sizeof(flags[0]);

	for (int i = 1; i < argc; i++) {
		if (flag(argv[i], flags, fc) != 0) {
			fprintf(stderr, "%s: unknown flag on \"%s\".\n", WHICH_KEYWORD_S, argv[i]);
			fflush(stderr);
			return KEY_FULL_ERR;
		}
	}

	if (help == true) {
		printf(
			"which built-in instructions:\n"
			"usage:\n"
			"\twhich -<flags> --<flags>\n"
			"\twhich will printf if the specified binary exists on the system, "
			"indicating if it's a binary, built-in, or alias.\n"
			"\n"

			"flags:\n"
			"\t-%c or --"WHICH_KEY_HELP_S": show this menu.\n"
			"\t-%c or --"WHICH_KEY_PATH_ONLY_S": only print the path of a binary.\n",
			WHICH_KEY_HELP_C,
			WHICH_KEY_PATH_ONLY_C
		);
		fflush(stdout);
		return KEY_SUCCESS;
	}

	bool       err = false;
	bool did_stuff = false;
	for (int i = 1; i < argc; i++) {
		if (argv[i][0] == '-') continue;

		did_stuff = true;
		arg_t arg = make_arg(argv[i]);

		// aliases
		int status = 0;
		arg = find_alias(arg, &status);

		if (status == 0) {
			// alias was found
			printf("\"%s\" <aliased to> -> \"%.*s\"\n", argv[i], (int)arg.len, arg.ptr);
			continue;
		}

		// get the stuff from path now
		char *resolved = fetch_from_path(arg);

		// keywords
		int key = find_keyword(argv[i]);
		if (key != NOT_A_KEYWORD) {
			printf("\"%s\" -> <built-in>\n", argv[i]);
			continue;
		}

		if (!resolved) {
			fprintf(stderr, "%s: unknown binary \"%s\".\n", WHICH_KEYWORD_S, argv[i]);
			err = true;

		} else {
			if (only_path) {
				printf("%s\n", resolved);

			} else {
				printf("\"%s\" -> \"%s\"\n", argv[i], resolved);
			}
		}
	}

	if (did_stuff == false) {
		fprintf(stderr, "%s: nothing was done!\n", WHICH_KEYWORD_S);
		err = true;
	}

	fflush(stdout);
	if (err == true) fflush(stderr);
	return err ? KEY_PARTIAL_ERR : KEY_SUCCESS;
}

int echo_keyword(int argc, char *argv[]) {
	int i = 1;
	while (i < argc) {
		printf("%s", argv[i++]);
		if (i < argc) putchar(' ');
	}

	putchar('\n');
	fflush(stdout);

	return KEY_SUCCESS;
}

int cd_keyword(int argc, char *argv[], char *envp[]) {
	if (argc > 2) {
		fprintf(stderr, "%s: excess arguments shall be ignored.\n", CD_KEYWORD_S);
		fflush(stderr);
	}

	bool err = false;

	// oldpwd
	char buf[PATH_MAX + 1] = {0};
	getcwd(buf, sizeof(buf) - 1);
	export_env("OLDPWD", buf);

	if (argc < 2) {
		// home dir, always
		if (chdir(envp_get(envp, "HOME")) != 0) {
			fprintf(stderr, "%s: couldn't change to home directory: %s\n",
				CD_KEYWORD_S, strerror(errno));
			err = true;
		}

	} else {
		// user-picked dir
		if (chdir(argv[1]) != 0) {
			fprintf(stderr, "%s: couldn't move to \"%s\": %s\n",
				CD_KEYWORD_S, argv[1], strerror(errno));
			err = true;
		}
	}

	memset(buf, '\0', sizeof(buf));
	getcwd(buf, sizeof(buf) - 1);
	export_env("PWD", buf);

	if (err) {
		fflush(stderr);
		return KEY_FULL_ERR;
	}

	return KEY_SUCCESS;
}

int pwd_keyword(void) {
	char path[PATH_MAX] = {0};
	getcwd(path, sizeof(path));
	printf("%.*s\n", (int)sizeof(path), path);
	fflush(stdout);
	return KEY_SUCCESS;
}

int env_keyword(char *envp[]) {
	while (*envp) {
		arg_t env = make_arg(*envp);

		// label and equal
		arg_t prev = cap_to_char(env, '=');
		prev.len++;

		// value, ignore the special treatment :P
		arg_t last = shift_arg(shift_arg_c(env, '='), 1);

		printf("%.*s\e[1m%.*s\e[0m\n",
			(int)prev.len, prev.ptr,
			(int)last.len, last.ptr);

		envp++;
	}
	fflush(stdout);
	return KEY_SUCCESS;
}

int export_keyword(int argc, char *argv[]) {
	if (argc < 2) {
		fprintf(stderr, "%s: not 'nuff arguments.\n", EXPORT_KEYWORD_S);
		fflush(stderr);
		return KEY_FULL_ERR;
	}

	bool err = false;
	for (int i = 1; i < argc; i++) {
		arg_t arg = make_arg(argv[i]);

		char *eq = arg_chr(arg, '=');
		if (eq == arg.ptr) {
			fprintf(stderr, "%s: malformed request for \"%s\".\n",
				EXPORT_KEYWORD_S, argv[i]);
			err = true;
			continue;
		}

		if (eq) *eq = '\0';

		if (export_env(argv[i], eq ? eq + 1 : NULL) != 0) {
			fprintf(stderr, "%s: failed to export environment variable \"%s\".\n",
				EXPORT_KEYWORD_S, argv[i]);
			err = true;
			continue;
		}

		if (!strncmp(argv[i], "PATH=", 5) || !strcmp(argv[i], "PATH")) refresh_path();

		if (eq) *eq = '=';
	}

	if (err == true) fflush(stderr);

	return err ? KEY_PARTIAL_ERR : KEY_SUCCESS;
}

int unset_keyword(int argc, char *argv[]) {
	bool err = false;
	if (argc < 2) {
		fprintf(stderr, "%s: not 'nuff arguments.\n", UNSET_KEYWORD_S);
		fflush(stderr);
		return KEY_FULL_ERR;
	}

	for (int i = 1; i < argc; i++) {
		fprintf(stderr, "");
		if (unset_env(argv[i]) != 0) {
			fprintf(stderr, "%s: failed to unset environment variable \"%s\".\n",
				EXPORT_KEYWORD_S, argv[i]);
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

	if (!i) printf("<no paths have been defined>\n");

	fflush(stdout);
	return KEY_SUCCESS;
}

int alias_keyword(int argc, char *argv[]) {
	bool err = false;
	if (argc < 2) {
		// print every alias
		size_t i = 0;
		arg_t *p = NULL;

		while ((p = reveal_alias(&i))) {
			if (p->ptr && p->asz) {
				// label and equal
				arg_t prev = cap_to_char(*p, '=');
				prev.len++;

				// replacement command
				arg_t last = shift_arg(*p, prev.len);

				printf("%.*s\"\e[1m%.*s\e[0m\"\n",
					(int)prev.len, prev.ptr,
					(int)last.len, last.ptr);
			}

			i++;
		}

		if (!i) printf("<no aliases set>\n");

		fflush(stdout);
		return KEY_SUCCESS;
	}

	for (int i = 1; i < argc; i++) {
		if (argv[i][0]== '=') {
			fprintf(stderr, "%s: malformed request for \"%s\".\n",
				ALIAS_KEYWORD_S, argv[i]);
			err = true;
			continue;
		}

		arg_t arg = make_arg(argv[i]);

		// make sure there's an equal sign
		char *eq = arg_chr(arg, '=');
		if (!eq) {
			fprintf(stderr, "%s: malformed request for \"%s\".\n",
				ALIAS_KEYWORD_S, argv[i]);
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

int unalias_keyword(int argc, char **argv) {
	bool err = false;
	if (argc < 2) {
		fprintf(stderr, "%s: not 'nuff arguments.\n", UNALIAS_KEYWORD_S);
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

int return_keyword(int argc, char **argv) {
	return argc < 2 ? 0 : strtol(argv[1], NULL, 0);
}
