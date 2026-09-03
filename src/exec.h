#ifndef EXEC_H
#define EXEC_H

// won't fork on execve()
#define NO_FORK   1 << 0
// status will always be 0
#define NO_STATUS 1 << 1

// just to check for invalid flags
#define INVALID_FLAG ~(NO_FORK & NO_STATUS)
// stuff that the user isn't at fault for
#define INTERNAL_ERR 127

// does both keywords
int execute(int argc, char *argv[], char *envp[], char flags);

// global argv[0]
extern char *global_argv0;

#endif
