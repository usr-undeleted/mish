#include <sys/stat.h>
#include <sys/wait.h>
#include <string.h>
#include <libgen.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>

#include "arg.h"
#include "keyword.h"
#include "path.h"
#include "exec.h"

char *global_argv0 = NULL;

int execute(int argc, char **argv, char **envp, char flags) {
	if (flags & INVALID_FLAG) {
		fprintf(stderr, "%s: execute() called with unknown flag (internal).\n",
			basename(global_argv0));
		fflush(stderr);
		return INTERNAL_ERR;
	}

	int status = 0;

	// try keywords
	int key = find_keyword(argv[0]);
	if (key != NOT_A_KEYWORD) {
		switch (key) {
			case EXIT_KEYWORD_N: {
				status = exit_keyword();
				break;
			}

			case WHICH_KEYWORD_N: {
				status = which_keyword(argc, argv);
				break;
			}

			case ECHO_KEYWORD_N: {
				status = echo_keyword(argc, argv);
				break;
			}

			case CD_KEYWORD_N: {
				status = cd_keyword(argc, argv, envp);
				break;
			}

			case PWD_KEYWORD_N: {
				status = pwd_keyword();
				break;
			}

			case ENV_KEYWORD_N: {
				status = env_keyword(envp);
				break;
			}

			case EXPORT_KEYWORD_N: {
				status = export_keyword(argc, argv);
				break;
			}

			case UNSET_KEYWORD_N: {
				status = unset_keyword(argc, argv);
				break;
			}

			case PATH_KEYWORD_N: {
				status = path_keyword();
				break;
			}

			case ALIAS_KEYWORD_N: {
				status = alias_keyword(argc, argv);
				break;
			}

			case UNALIAS_KEYWORD_N: {
				status = unalias_keyword(argc, argv);
				break;
			}

			case RETURN_KEYWORD_N: {
				status = return_keyword(argc, argv);
				break;
			}

			case HASH_KEYWORD_N: {
				status = hash_keyword();
				break;
			}

			default: {
				status = INTERNAL_ERR;
				fprintf(stderr, "%s: unhandled keyword %d (internal).\n",
					basename(global_argv0), key);
				fflush(stderr);
				break;
			}
		}

		return status;
	}

	char *bin_path = fetch_from_path(make_arg(argv[0]));
	if (!bin_path) {
		// see if file even exists in the first place, if its just a path
		struct stat dummy;
		if (stat(argv[0], &dummy) != 0) {
			fprintf(stderr, "%s: couldn't execute \"%s\": Unknown binary\n",
				basename(global_argv0), argv[0]);
			fflush(stderr);
			return flags & NO_STATUS ? 0 : INTERNAL_ERR;

		} else {
			bin_path = argv[0];
		}
	}

	if (!(flags & NO_FORK)) {
		pid_t child = fork();
		switch (child) {
			case -1: {
				return 1;
				break;
			}

			case 0: {
				goto exec;
				break;
			}

			default: {
				while (waitpid(child, &status, WNOHANG) != -1) {};
				break;
			}
		}
	} else {
		goto exec;
	}

	if (flags & NO_STATUS) status = 0;
	return status;

	// what a fork would do
	exec:
	execve(bin_path, argv, envp);

	fprintf(stderr, "%s: couldn't execute \"%s\": %s\n",
		basename(global_argv0), bin_path, strerror(errno));
   				fflush(stderr);
	return flags & NO_STATUS ? 0 : INTERNAL_ERR;
}
