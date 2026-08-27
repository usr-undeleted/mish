#ifndef KEYWORD_H
#define KEYWORD_H

#include <stdbool.h>

// relates a string to a keyword
int find_keyword(const char *str);

void exit_keyword(void);

void which_keyword(const int argc, const char *argv[]);

#define NOT_A_KEYWORD 0

#define EXIT_KEYWORD_S "exit"
#define EXIT_KEYWORD_N 1

#define WHICH_KEYWORD_S "which"
#define WHICH_KEYWORD_N 2

#endif
