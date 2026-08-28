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

// list of arg_ts with how much memory was allocated
// finding the number of usable slots in the array would
// be with dividing the alloc_sz with sizeof(arg_t)
typedef struct {
    // alloc count
    size_t      asz;
    // item count
    size_t     icnt;
    arg_t      *ptr;
} arg_arr_t;

// make a new arg, going to the nearest whitespace
arg_t goto_whitespace(const arg_t arg);

// make a new arg, skipping whitespace
arg_t skip_whitespace(const arg_t arg);

// decrease the args length till it encapsulates
// everything not in whitespace
arg_t cap_to_white(const arg_t arg);

// (re)allocates a pointer for an arg
int alloc_arg(arg_t *arg, const size_t sz);

// see if an arg is empty
// return 1 on yes
int arg_empty(arg_t arg);

// (re)allocate a arg_t pointer for a list
int alloc_arg_arr(arg_arr_t *arr, size_t sz);

// compare two args
// same logic as strcmp
int arg_cmp(const arg_t one, const arg_t two);

// make an arg from just a string
arg_t make_arg(const char *str);

// free an arg's memory (if its allocated)
// otherwise, just zero out memory
void free_arg(arg_t *arg);

#endif
