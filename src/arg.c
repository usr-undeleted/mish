#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "arg.h"

#define MAX(x, y) x > y ? x : y

// make a new arg, going to the nearest whitespace
arg_t goto_whitespace(const arg_t arg) {
	arg_t ret = arg;

	while (!isspace(*ret.ptr) && ret.len) {
		ret.ptr++;
		ret.len--;
	}

	return ret;
}

// make a new arg, skipping whitespace
arg_t skip_whitespace(const arg_t arg) {
	arg_t ret = arg;

	while (isspace(*ret.ptr) && ret.len) {
		ret.ptr++;
		ret.len--;
	}

	return ret;
}

// cap the args length to the nearest whitespace
arg_t cap_to_white(const arg_t arg) {
	arg_t  ret = arg;
	size_t i = 0;

	while (!isspace(ret.ptr[i]) && i < arg.len) {
		i++;
	}

	ret.len = i;

	return ret;
}

// truncate an arg to a specific char
arg_t trunc_to_char(const arg_t arg, const char c) {
	arg_t  ret = arg;
	size_t i = arg.len;

	while (arg.ptr[i] != c && i > 0) {
		i--;
	}

	ret.len = i;

	return ret;
}

// (re)allocates a pointer for an arg
int alloc_arg(arg_t *arg, const size_t sz) {
	// if it doesn't exist
	if (!arg->ptr) {
		arg->ptr = calloc(sizeof(char), sz);
		if (!arg->ptr) return 1;

		arg->len = 0;
		arg->asz = sz;

		return 0;
	}

	// else, add to allocation
	arg->ptr = realloc(arg->ptr, arg->asz + sz);
	if (!arg->ptr) return 1;

	memset(arg->ptr + arg->asz, '\0', sz);
	arg->asz += sz;

	return 0;
}

// see if an arg is empty
// return 1 on yes
int arg_empty(arg_t arg) {
	for (size_t i = 0; i < arg.len; i++) {
		if (!isspace(arg.ptr[i])) return 0;
	}

	return 1;
}
#include <stdio.h>
// (re)allocate a arg_t pointer for a list
int alloc_arg_arr(arg_arr_t *arr, size_t sz) {
    if (!arr) {
        arr = calloc(sizeof(arg_t), sz);
        if (!arr) return 1;

    } else {
        arr->ptr = realloc(arr->ptr, arr->asz + sz);
        if (!arr->ptr) return 1;
        arr->asz += sz;
    }

    return 0;
}

// compare two args
// same logic as strcmp
int arg_cmp(const arg_t one, const arg_t two) {
    if (one.len != two.len) return 1;

    if (strncmp(one.ptr, two.ptr, MAX(one.len, two.len))) return 1;

    return 0;
}

// make an arg from just a string
arg_t make_arg(const char *str) {
    size_t len = strlen(str);

    arg_t ret = {
        .ptr = (char *)str,
        .len = len,
        .asz = 0,
    };

    return ret;
}

void free_arg(arg_t *arg) {
	if (arg->asz) free(arg->ptr);
	memset(arg, '\0', sizeof(arg_t));
}
