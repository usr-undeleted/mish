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

    } else {
        return NOT_A_KEYWORD;
    }
}

void exit_keyword(void) {
    exit(0);
}

void which_keyword(const int argc, const char **argv) {
	if (argc < 2) {
		fprintf(stderr, "%s: too little arguments.\n", WHICH_KEYWORD_S);
		fflush(stderr);
		return;
	}

	for (int i = 1; i < argc; i++) {
		char *resolved = fetch_from_path(make_arg(argv[i]));

		int key = find_keyword(argv[i]);
		if (key != NOT_A_KEYWORD) {
			printf("%s -> <built-in>\n", argv[i]);
			continue;
		}

		if (!resolved) {
			printf("%s: unknown binary \"%s\".\n", WHICH_KEYWORD_S, argv[i]);

		} else {
			printf("%s -> %s\n", argv[i], resolved);

		}
	}

	fflush(stdout);
}

void echo_keyword(const int argc, const char **argv) {
	int i = 1;
	while (i < argc) {
		printf("%s", argv[i]);

		i++;

		if (i < argc) putchar(' ');
	}

	putchar('\n');

	fflush(stdout);
}

void cd_keyword(const int argc, const char **argv) {
	if (argc < 2) {
		// no args leads directly to home
		char *home = getenv("HOME");
		if (!home) {
			fprintf(stderr, "%s: failed to get home directory path.\n",
				CD_KEYWORD_S);
			return;
		}

		if (chdir(home) != 0) {
			fprintf(stderr, "%s: couldn't change to \"%s\": %s\n",
				CD_KEYWORD_S, argv[1], strerror(errno));
		}

		return;

	} else if (argc > 2) {
		fprintf(stderr, "%s: excess arguments shall be ignored.\n", CD_KEYWORD_S);
	}

	if (chdir(argv[1]) != 0) {
		fprintf(stderr, "%s: couldn't change to \"%s\": %s\n",
			CD_KEYWORD_S, argv[1], strerror(errno));
	}

	fflush(stderr);
}

void pwd_keyword(void) {
	char path[PATH_MAX] = {0};
	getcwd(path, sizeof(path));
	printf("%.*s\n", (int)sizeof(path), path);
	fflush(stdout);
}

void env_keyword(const char **envp) {
	while (*envp) {
		printf("%s\n", *envp);
		envp++;
	}
	fflush(stdout);
}
