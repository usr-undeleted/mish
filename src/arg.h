#ifndef ARG_H
#define ARG_H

#include <stddef.h>

// *_alloc_sz macros to define how much
// to allocate for at once
#define ARGV_ASZ 16

// string should include newline
typedef struct {
	char  *ptr;
	size_t len;
	// allocation size
	size_t asz;

} arg_t;

// make a new arg, going to the nearest whitespace
arg_t goto_whitespace(const arg_t arg);

// make a new arg, skipping whitespace
arg_t skip_whitespace(const arg_t arg);

// decrease the args length till it encapsulates
// everything not in whitespace
arg_t trunc_to_white(const arg_t arg);

// (re)allocates a pointer for an arg
int alloc_arg(arg_t *arg, const size_t sz);

// see if an arg is empty
// return 1 on yes
int arg_empty(arg_t arg);

#endif
