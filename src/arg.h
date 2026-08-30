#ifndef ARG_H
#define ARG_H

#include <stddef.h>
#include <ctype.h>

#define MAX(x, y) x > y ? x : y
#define MIN(x, y) x < y ? x : y

// check if a char is empty
#define EMPTY_C(c) (isspace(c) ? 1 : iscntrl(c) ? 1 : 0)

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
// this allocates the number of indexes on the list,
// not the actual items
int alloc_arg_arr(arg_arr_t *arr, size_t sz);

// compare two args
// same logic as strcmp
int arg_cmp(const arg_t one, const arg_t two);

// make an arg from just a string
arg_t make_arg(const char *str);

// free an arg's memory (if its allocated)
// otherwise, just zero out memory
void free_arg(arg_t *arg);

// doesn't free, only zeroes out memory
void zero_arg(arg_t *arg);

// same logic as zero_arg
// also sets icnt to 0
void zero_arg_arr(arg_arr_t *arr);

// free every item on array
void free_arg_arr(arg_arr_t *arr);

// copy an arg to another, automatically
// (re)allocating memory
int arg_cpy(arg_t *dest, arg_t src);

// like strchr, but the field is
// an arg (limits itself to the length)
char *arg_chr(const arg_t arg, const char ch);

// return a copy of the arg with a specific
// offset, shifting to the right only
// note that this shifts the pointer to
// at max its length
arg_t shift_arg(arg_t arg, size_t off);

// move an index of an arg to the right until
// it hits an empty character
void idx_to_white(size_t *i, const arg_t arg);

// cap an arg to a specific char
arg_t cap_to_char(const arg_t arg, const char c);

// same logic as shift_arg(), but shifts
// until a specific char (or until it can't anymore)
arg_t shift_arg_c(arg_t arg, const char c);

// append an arg to another, allocating if needed
int append_arg(arg_t *dest, const arg_t src);

#endif
