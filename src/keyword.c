#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libgen.h>
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
