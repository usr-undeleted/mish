#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "arg.h"

// make a new arg, going to the nearest whitespace
inline arg_t goto_whitespace(const arg_t arg) {
	arg_t ret = arg;
	char quote_type = NO_QUOTES;

	while (ret.len) {
		char q = QUOTE_T(*ret.ptr);

		// as much as i love switches, they'd be
		// even more indentation hell than what i
		// already have in here :p
		if (q != NO_QUOTES) {
			if (quote_type != NO_QUOTES) {
				if (quote_type == q) quote_type = NO_QUOTES;

			} else {
				quote_type = q;
			}

		} else if (*ret.ptr == '\\' && quote_type == NO_QUOTES) {
			// since we are outside quotes, we
			// skip a character
			ret.ptr++;
			ret.len--;
			ret.asz--;

		} else if (quote_type == NO_QUOTES && EMPTY_C(*ret.ptr)) break;

		if (!ret.len) break;

		ret.ptr++;
		ret.len--;
		ret.asz--;
	}

	return ret;
}

// make a new arg, skipping whitespace
inline arg_t skip_whitespace(const arg_t arg) {
	arg_t ret = arg;

	while (isspace(*ret.ptr) && ret.len) {
		ret.ptr++;
		ret.len--;
		ret.asz--;
	}

	return ret;
}

// cap the args length to the nearest whitespace
inline arg_t cap_to_white(const arg_t arg) {
	arg_t  ret = arg;
	size_t i = 0;
	char quote_type = NO_QUOTES;

	while (i < arg.len) {
		char q = QUOTE_T(ret.ptr[i]);

		if (q != NO_QUOTES) {
			if (quote_type != NO_QUOTES) {
				if (quote_type == q) quote_type = NO_QUOTES;

			} else {
				quote_type = q;
			}

		} else if (ret.ptr[i] == '\\' && quote_type == NO_QUOTES) {
			i++;

		} else if (EMPTY_C(ret.ptr[i]) && quote_type == NO_QUOTES) break;

		if (i >= arg.len) break;
		i++;
	}

	ret.len = i;
	ret.asz = i;

	return ret;
}

// cap an arg to a specific char
inline arg_t cap_to_char(const arg_t arg, const char c) {
	arg_t  ret = arg;
	size_t i = 0;

	while (arg.ptr[i] != c && i < arg.len) {
		i++;
	}

	ret.len = i;
	ret.asz = i;

	return ret;
}

// (re)allocates a pointer for an arg
inline bool alloc_arg(arg_t *arg, const size_t sz) {
	if (!arg) return 1;

	arg->ptr = realloc(arg->ptr, arg->asz + sz);
	if (!arg->ptr) return 1;

	memset(arg->ptr + arg->asz, '\0', sz);
	arg->asz += sz;

	return 0;
}

// see if an arg is empty
// return 1 on yes
inline bool arg_empty(arg_t arg) {
	for (size_t i = 0; i < arg.len; i++) {
		if (!EMPTY_C(arg.ptr[i]) || !arg.ptr[i]) return 0;
	}

	return 1;
}

// (re)allocate a arg_t pointer for a list
// multiplies sz by sizeof(arg_t)
inline bool alloc_arg_arr(arg_arr_t *arr, size_t sz) {
	if (!arr) return 1;

	arr->ptr = realloc(arr->ptr, (arr->asz + sz) * sizeof(arg_t));
    if (!arr->ptr) return 1;

    memset(arr->ptr + arr->asz, '\0', sz);
    arr->asz += sz;

    return 0;
}

// compare two args
// same logic as strcmp
inline bool arg_cmp(const arg_t one, const arg_t two) {
    if (one.len != two.len) return 1;

    if (strncmp(one.ptr, two.ptr, MAX(one.len, two.len))) return 1;

    return 0;
}

// make an arg from just a string
inline arg_t make_arg(const char *str) {
    size_t len = strlen(str);

    arg_t ret = {
        .ptr = (char *)str,
        .len = len,
        .asz = len,
    };

    return ret;
}

inline void free_arg(arg_t *arg) {
	if (arg->ptr && arg->asz) free(arg->ptr);
	memset(arg, '\0', sizeof(arg_t));
}

// doesn't free, only zeroes out memory
inline void zero_arg(arg_t *arg) {
	if (arg->ptr) memset(arg->ptr, '\0', arg->asz);
	arg->len = 0;
}

// same logic as zero_arg
// also sets icnt to 0
inline void zero_arg_arr(arg_arr_t *arr) {
	for (size_t i = 0; i < arr->icnt; i++) {
		zero_arg(&arr->ptr[i]);
	}
	arr->icnt = 0;
}

// free every item on array
inline void free_arg_arr(arg_arr_t *arr) {
	for (size_t i = 0; i < arr->icnt; i++) {
		if (arr->ptr) free(arr->ptr);
	}

	arr->icnt = 0;
}

// copy an arg to another, automatically
// (re)allocating memory
inline bool arg_cpy(arg_t *dest, const arg_t src) {
	if (!dest) return 1;

	if (src.len > dest->asz) {
		if (alloc_arg(dest, src.len + 1)) return 1;
		dest->asz = src.len + 1;
	}

	memcpy(dest->ptr, src.ptr, src.len);
	dest->len = src.len;

	return 0;
};

// like strchr, but the field is
// an arg (limits itself to the length)
inline char *arg_chr(const arg_t arg, const char ch) {
	for (size_t i = 0; i < arg.len; i++) {
		if (arg.ptr[i] == ch) return arg.ptr + i;
	}

	return NULL;
}

// return a copy of the arg with a specific
// offset, shifting to the right only
// note that this shifts the pointer to
// at max its length
inline arg_t shift_arg(arg_t arg, const size_t off) {
	arg.len -= MIN(arg.len, off);
	arg.asz -= MIN(arg.len, off);
	arg.ptr += MIN(arg.len, off);

	return arg;
}

// same logic as shift_arg(), but shifts
// until a specific char (or until it can't anymore)
inline arg_t shift_arg_c(arg_t arg, const char c) {
	size_t i = 0;

	while (i < arg.len && arg.ptr[i] != c) i++;

	arg.ptr += i;
	arg.len -= i;
	arg.asz -= i;

	return arg;
}

// move an index of an arg to the right until
// it hits an empty character
inline void idx_to_white(size_t *i, const arg_t arg) {
	if (!i) return;

	while (*i < arg.len && !EMPTY_C(arg.ptr[*i])) (*i)++;
}

// append an arg to another, allocating if needed
inline bool append_arg(arg_t *dest, const arg_t src) {
	if (!dest) return 1;

	if ((dest->len + src.len) > dest->asz) {
		if (alloc_arg(dest, (dest->len + src.len) - dest->asz)) return 1;
	}

	memcpy(dest->ptr + dest->len, src.ptr, src.len);
	dest->len += src.len;

	return 0;
}

inline bool append_arg_to_arr(arg_arr_t *arr, const arg_t item, const size_t sz) {
	if (!arr) return 1;
	// find usable idx
	size_t i = 0;
	while (i < arr->icnt && arr->ptr[i].asz != 0) i++;

	if (i == arr->icnt) {
		// if the idx is at the end of the list, and we would need more allocation
		if ((arr->icnt + 1) >= (arr->asz / sizeof(arg_t))) {
			if (alloc_arg_arr(arr, sz)) return 1;
		}

		zero_arg(&arr->ptr[arr->icnt]);
		if (append_arg(&arr->ptr[arr->icnt], item)) return 1;

		++arr->icnt;

	} else {
		// item in middle of list
		zero_arg(&arr->ptr[i]);
		if (append_arg(&arr->ptr[i], item)) return 1;
	}

	return 0;
}
