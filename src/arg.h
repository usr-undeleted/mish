#ifndef ARG_H
#define ARG_H

#include <stdbool.h>
#include <stddef.h>
#include <ctype.h>

#define MAX(x, y) x > y ? x : y
#define MIN(x, y) x < y ? x : y

// check if a char is empty
#define EMPTY_C(c) (isspace(c) ? 1 : iscntrl(c) ? 1 : 0)

#define NO_QUOTES 0
#define QUOTE_DBL 1
#define QUOTE_SIN 2

// get the quote type of a char
#define QUOTE_T(c) (c == '\"' ? QUOTE_DBL : c == '\'' ? QUOTE_SIN : NO_QUOTES)
// takes in a char to see if its either '/', '.', or '~', aka a path
#define IS_A_PATH(c) (c == '/' ? 1 : c == '.' ? 1 : c == '~' ? 1 : 0)

// *_alloc_sz macros to define how much
// to allocate for at once
#define ARGV_ASZ 16

#define ARGV_ARR_ASZ 8

// see if either the asz or ptr of an arg is 0
#define ARG_NULL(arg) (!arg.ptr ? 1 : !arg.asz ? 1 : 0)
#define ARG_NULL_P(arg) (!arg->ptr ? 1 : !arg->asz ? 1 : 0)

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

// replaces the arg_t list with a generic double pointer
typedef struct {
	char   **dp;
	size_t  asz;
	size_t icnt;

} dbl_ptr_t;

// make a new arg, going to the nearest whitespace
arg_t goto_whitespace(const arg_t arg);

// make a new arg, skipping whitespace
arg_t skip_whitespace(const arg_t arg);

// decrease the args length till it encapsulates
// everything not in whitespace
arg_t cap_to_white(const arg_t arg);

// (re)allocates a pointer for an arg
bool alloc_arg(arg_t *arg, const size_t sz);

// see if an arg is empty
// return 1 on yes
bool arg_empty(arg_t arg);

// (re)allocate a arg_t pointer for a list
// this allocates the number of indexes on the list,
// not the actual items
bool alloc_arg_arr(arg_arr_t *arr, size_t sz);

// compare two args
// same logic as strcmp
bool arg_cmp(const arg_t one, const arg_t two);

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
bool arg_cpy(arg_t *dest, arg_t src);

// like strchr, but the field is
// an arg (limits itself to the length)
char *arg_chr(const arg_t arg, const char ch);

// reverse search (arg_chr)
char *arg_r_chr(const arg_t arg, const char ch);

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
bool append_arg(arg_t *dest, const arg_t src);

// append an arg_t to an arg_arr_t
//
// will overwrite entries with asz == 0
//
// sz relates to the allocation size used
bool append_arg_to_arr(arg_arr_t *arr, const arg_t item, const size_t sz);

// remove an entry from an arr by simply setting it
// as usable (asz = 0)
//
// 0 on match, 1 on no match
bool free_arr_entry(arg_arr_t *arr, const arg_t arg);

// "shift" variant of free(), using memmove()
// to completely overwrite the old entry away.
bool s_free_arr_entry(arg_arr_t *arr, const arg_t arg);

// same idea as s_free_arr_entry, but doesn't free and works
// on a double pointer array type
bool s_remove_dbpl_entry(dbl_ptr_t *arr, const char *str);

// same as s_remove_dbpl_entry, but stops string comparison at a char
bool s_remove_dbpl_entry_c(dbl_ptr_t *arr, const char *str, const char c);

// generic freeing of a double pointer list
void free_dblp(dbl_ptr_t *arr);

// shorten the length of an arg
//
// if sz is too big for one of the length, set
// it to zero
arg_t shorten_arg(arg_t arg, const size_t sz);

// use strdup to add an entry to a dbl_ptr_t
bool append_to_dblp(dbl_ptr_t *arr, const void *p, const size_t sz);

// get the length of a string up to a char
size_t str_len_c(const char *str, const char c);

// compare two strings only up until a char
//
// return 0 on exact match
bool str_cmp_c(const char *one, const char *two, const char c);

// find an entry inside of a dblp
//
// sets status to true if it succeded, else, false
size_t find_dblp_entry(const dbl_ptr_t dblp, const char *label, bool *status);

// same logic as find_dblp_entry, but limits itself
// to a specific char
size_t find_dblp_entry_c(const dbl_ptr_t dblp, const char *label, bool *status, const char c);

// get a pointer from a double pointer
char *dblp_get_ptr(const dbl_ptr_t arr, const char *label);

// get a pointer from a double pointer, with char stuff
char *dblp_get_ptr_c(const dbl_ptr_t arr, const char *label, const char c);

// sees if an arg is a definition, as in
bool arg_is_def(const arg_t arg);

// make the struct from a double pointer
//
// sets asz to len * sizeof(dp[0])
dbl_ptr_t make_dblp(char **dp);

// find the equivalent closer for the opener
//
// returns null on failure to find the closer, or when the
// pointer provided isn't the open char
char *find_closer(arg_t arg, const char open, const char close);

// decides what to do with quotes on a char
//
// returns 1 if the callee should increment their index
bool determine_quote(const char ch, char *quote_type);

// libgen's basename
arg_t arg_basename(const arg_t arg);

#endif
