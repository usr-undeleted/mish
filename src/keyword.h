#ifndef KEYWORD_H
#define KEYWORD_H

#include <stdbool.h>

#include "hash.h"

// used by main.c to start the hash map
extern arg_arr_t builtin_list;
// hash map for keywords
extern hash_map builtin_hash;

// relates a string to a keyword
int find_keyword(char *str);

int exit_keyword(void);

int which_keyword(int argc, char *argv[]);

int echo_keyword(int argc, char *argv[]);

int cd_keyword(int argc, char *argv[], char *envp[]);

int pwd_keyword(void);

int env_keyword(char **envp);

int export_keyword(int argc, char *argv[]);

int unset_keyword(int argc, char *argv[]);

int path_keyword(void);

int alias_keyword(int argc, char *argv[]);

int unalias_keyword(int argc, char *argv[]);

int return_keyword(int argc, char *argv[]);

int hash_keyword(void);

#define NOT_A_KEYWORD 0

#define EXIT_KEYWORD_S "exit"
#define EXIT_KEYWORD_N 1

#define WHICH_KEYWORD_S "which"
#define WHICH_KEYWORD_N 2

#define ECHO_KEYWORD_S "echo"
#define ECHO_KEYWORD_N 3

#define CD_KEYWORD_S "cd"
#define CD_KEYWORD_N 4

#define PWD_KEYWORD_S "pwd"
#define PWD_KEYWORD_N 5

#define ENV_KEYWORD_S "env"
#define ENV_KEYWORD_N 6

#define EXPORT_KEYWORD_S "export"
#define EXPORT_KEYWORD_N 7

#define UNSET_KEYWORD_S "unset"
#define UNSET_KEYWORD_N 8

#define PATH_KEYWORD_S "path"
#define PATH_KEYWORD_N 9

#define ALIAS_KEYWORD_S "alias"
#define ALIAS_KEYWORD_N 10

#define UNALIAS_KEYWORD_S "unalias"
#define UNALIAS_KEYWORD_N 11

#define RETURN_KEYWORD_S "return"
#define RETURN_KEYWORD_N 12

#define HASH_KEYWORD_S "hash"
#define HASH_KEYWORD_N 13

#endif
