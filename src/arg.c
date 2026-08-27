#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "arg.h"

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

// decrease the args length till it encapsulates
// everything not in whitespace
arg_t trunc_to_white(const arg_t arg) {
	arg_t  ret = arg;
	size_t i = 0;

	while (!isspace(ret.ptr[i]) && i < ret.len) {
		i++;
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
