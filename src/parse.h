#ifndef PARSE_H
#define PARSE_H

#include <stddef.h>

#include "arg.h"

// remake an arg (like input), doing the following:
//
// 1. replacing the immediate first argument with an alias
// 2. (to be added) globbing files
// 3. consuming args that are DEFS=something
int remake_arg(arg_t *dest, const arg_t src);

// parse an arg into an arg_arr_t
bool make_child_argv(const arg_t arg, arg_arr_t *child_argv, int *child_argc, size_t asz);

#endif
