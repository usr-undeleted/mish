#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "arg.h"

// decides what to do with quotes on a char
//
// returns 1 if the callee should increment their index
bool determine_quote(const char ch, char *quote_type) {
	char quote_c = QUOTE_T(ch);

	if (quote_c != NO_QUOTES) {
		if (*quote_type != NO_QUOTES) {
			if (*quote_type == quote_c) *quote_type = NO_QUOTES;

		} else {
			*quote_type = quote_c;
		}

		return 1;
	}

	return 0;
}

// form an index that could be used, for example, to
// go to whitespace, or encapsulate an arg
size_t arg_skip_i(const arg_t arg) {
	char quote_type = NO_QUOTES;
	bool back = false;
	size_t ret = 0;

	while (ret < arg.len) {
		if (back) {
			back = false;
			goto end;
		}

		if (determine_quote(arg.ptr[ret], &quote_type)) {
			++ret;

		} else if (arg.ptr[ret] == '\\' && quote_type == NO_QUOTES) {
			back = true;

		} else if (arg.ptr[ret] == '$' && quote_type == NO_QUOTES) {
			ret++;
			char cl = 0;

			switch (arg.ptr[ret]) {
				case '(': {
					cl = ')';
					break;
				}

				case '[': {
					cl = ']';
					break;
				}

				default: {
					--ret;
					goto end;
				}
			}

			while (ret < arg.len && arg.ptr[ret] != cl) ret++;

		} else if (EMPTY_C(arg.ptr[ret]) && quote_type == NO_QUOTES) break;

		end:
		if (ret >= arg.len) break;
		ret++;
	}

	return ret;
}

// make a new arg, going to the nearest whitespace
inline arg_t goto_whitespace(const arg_t arg) {
	arg_t ret = arg;
	size_t i = arg_skip_i(arg);
	ret.ptr += i;
	ret.len -= i;
	ret.asz -= i;

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
	size_t i = arg_skip_i(arg);
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
	if (!arg.len) return 1;

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
    size_t len = str ? strlen(str) : 0;

    arg_t ret = {
        .ptr = (char *)str,
        .len = len,
        .asz = len,
    };

    return ret;
}

inline void free_arg(arg_t *arg) {
	if (ARG_NULL_P(arg)) free(arg->ptr);
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
}

// like strchr, but the field is
// an arg (limits itself to the length)
inline char *arg_chr(const arg_t arg, const char ch) {
	for (size_t i = 0; i < arg.len; i++) {
		if (arg.ptr[i] == ch) return arg.ptr + i;
	}

	return NULL;
}

// reverse search (arg_chr)
inline char *arg_r_chr(const arg_t arg, const char ch) {
	if (!arg.len) return NULL;

	for (size_t i = arg.len; i > 0; i--) {
		if (arg.ptr[i - 1] == ch) return arg.ptr + i - 1;
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

// an idx may be at the end of the list or
// as an entry with asz = 0 in the middle
size_t arr_usable_idx(const arg_arr_t arr) {
	size_t i = 0;
	while (i < arr.icnt && arr.ptr[i].asz != 0) i++;
	return i;
}

// append an arg to another, allocating if needed
inline bool append_arg(arg_t *dest, const arg_t src) {
	if (!dest) return 1;

	if ((dest->len + src.len) > dest->asz || !dest->ptr) {
		size_t n = ((dest->len + src.len) < dest->asz) ?
			dest->asz : (dest->len + src.len) - dest->asz;
		if (alloc_arg(dest, n)) return 1;
	}

	memcpy(dest->ptr + dest->len, src.ptr, src.len);
	dest->len += src.len;

	return 0;
}

inline bool append_arg_to_arr(arg_arr_t *arr, const arg_t item, const size_t sz) {
	if (!arr) return 1;
	// find usable idx
	size_t i = arr_usable_idx(*arr);

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

// free an entry from an arr by simply setting it
// as usable (asz = 0)
//
// 0 on match, 1 on no match
inline bool free_arr_entry(arg_arr_t *arr, const arg_t arg) {
	for (size_t i = 0; i < arr->icnt; i++) {
		if (!arg_cmp(arr->ptr[i], arg)) {
			free_arg(&arr->ptr[i]);
			if (i == (arr->icnt - 1)) --arr->icnt;

			return 0;
		}
	}

	return 1;
}

// "shift" variant of free(), using memmove()
// to completely overwrite the old entry away.
inline bool s_free_arr_entry(arg_arr_t *arr, const arg_t arg) {
	for (size_t i = 0; i < arr->icnt; i++) {
		if (!arg_cmp(arr->ptr[i], arg)) {
			free_arg(&arr->ptr[i]);
			memmove(&arr->ptr[i], &arr->ptr[i + 1], (arr->icnt - i) * sizeof(arg_t));
			--arr->icnt;
			return 0;
		}
	}

	return 1;
}

// get the length of a string up to a char
size_t str_len_c(const char *str, const char c) {
	size_t i = 0;
	while (*str != c && *str) {
		str++;
		i++;
	}

	return i;
}

// compare two strings only up until a char
//
// return 0 on exact match
bool str_cmp_c(const char *one, const char *two, const char c) {
	size_t len = str_len_c(one, c);
	if (len != str_len_c(two, c)) return 1;

	for (size_t i = 0; i < len; i++) {
		if (one[i] != two[i]) return 1;
	}

	return 0;
}

// find an entry inside of a dblp
//
// sets status to true if it succeded, else, false
size_t find_dblp_entry(const dbl_ptr_t dblp, const char *label, bool *status) {
	if (status) *status = false;

	for (size_t i = 0; i < dblp.icnt; i++) {
		if (!strcmp(dblp.dp[i], label)) {
			if (status) *status = true;
			return i;
		}
	}

	return 0;
}

// same logic as find_dblp_entry, but limits itself
// to a specific char
size_t find_dblp_entry_c(const dbl_ptr_t dblp, const char *label, bool *status, const char c) {
	if (status) *status = false;

	for (size_t i = 0; i < dblp.icnt; i++) {
		if (!str_cmp_c(dblp.dp[i], label, c)) {
			if (status) *status = true;
			return i;
		}
	}

	return 0;
}

// generic freeing of a double pointer list
inline void free_dblp(dbl_ptr_t *arr) {
	for (size_t i = 0; i < arr->icnt; i++) {
		if (arr->dp[i]) free(arr->dp[i]);
		arr->dp[i] = NULL;
	}
	arr->icnt = 0;
}

// append to a dblp array a pointer (only allocates size in the array)
//
// if p is NULL, add it, but don't increment icnt
inline bool append_to_dblp(dbl_ptr_t *arr, const void *p, const size_t sz) {
	// increase size if needed
	if ((arr->icnt + 1) >= (arr->asz / sizeof(arr->dp[0]))) {
		if (!(arr->dp = realloc(arr->dp, arr->asz + (sz * sizeof(arr->dp[0]))))) return 1;
		memset(arr->dp + arr->asz, '\0', sz);
		arr->asz += sz * sizeof(arr->dp[0]);
	}

	arr->dp[arr->icnt] = (void *)p;
	if (p) arr->icnt++;

	return 0;
}

// shorten the length of an arg
//
// if sz is too big for one of the length, set
// it to zero
inline arg_t shorten_arg(arg_t arg, const size_t sz) {
	if (sz >= arg.len) arg.len -= sz;
	else arg.len = 0;

	return arg;
}

inline bool s_remove_dbpl_entry(dbl_ptr_t *arr, const char *str) {
	if (!arr) return 1;

	bool exists = false;
	size_t i = find_dblp_entry(*arr, str, &exists);
	if (exists == false) return 1;

	memmove(&arr->dp[i], &arr->dp[i + 1], (arr->icnt - i) * sizeof(arr->dp[0]));
	--arr->icnt;

	return 1;
}

inline bool s_remove_dbpl_entry_c(dbl_ptr_t *arr, const char *str, const char c) {
	if (!arr) return 1;

	bool exists = false;
	size_t i = find_dblp_entry_c(*arr, str, &exists, c);
	if (exists == false) return 1;

	memmove(&arr->dp[i], &arr->dp[i + 1], (arr->icnt - i) * sizeof(arr->dp[0]));
	--arr->icnt;

	return 1;
}

// get a pointer from a double pointer
char *dblp_get_ptr(const dbl_ptr_t arr, const char *label) {
	bool exists = false;
	size_t i = find_dblp_entry(arr, label, &exists);

	if (exists) return arr.dp[i];
	else return NULL;
}

// get a pointer from a double pointer, with char stuff
char *dblp_get_ptr_c(const dbl_ptr_t arr, const char *label, const char c) {
	bool exists = false;
	size_t i = find_dblp_entry_c(arr, label, &exists, c);

	if (exists) return arr.dp[i];
	else return NULL;
}

// sees if an arg is a definition, as in something like "TEST=blah"
inline bool arg_is_def(const arg_t arg) {
	size_t i = 0;
	char quote_type = NO_QUOTES;

	while (i < arg.len) {
		char q = QUOTE_T(arg.ptr[i]);

		if (q != NO_QUOTES) {
			if (quote_type != NO_QUOTES) {
				if (quote_type == q) quote_type = NO_QUOTES;

			} else {
				quote_type = q;
			}

		} else if (arg.ptr[i] == '\\' && quote_type == NO_QUOTES) {
			if (!arg.ptr[++i]) break;

		} else if (arg.ptr[i] == '=' && i && quote_type == NO_QUOTES) return true;

		++i;
	}

	return false;
}

// make the struct from a double pointer
//
// sets asz to len * sizeof(dp[0])
dbl_ptr_t make_dblp(char **dp) {
	dbl_ptr_t ret = {0};
	ret.dp = dp;

	size_t i = 0;
	while (ret.dp[i++]);

	ret.icnt = i;
	ret.asz = ret.icnt * sizeof(ret.dp[0]);

	return ret;
}

// find the equivalent closer for the opener
//
// returns null on failure to find the closer, or when the
// pointer provided isn't the open char
char *find_closer(arg_t arg, const char open, const char close) {
	if (*arg.ptr != open) return NULL;

	size_t depth = 0;
	size_t i     = 0;

	while (i < arg.len) {
		if (arg.ptr[i] == open) {
			depth++;
		} else if (arg.ptr[i] == close) {
			depth--;
		}

		if (!depth) return &arg.ptr[i];

		i++;
	}

	if (depth) return arg_r_chr(arg, close);

	return NULL;
}

arg_t arg_basename(const arg_t arg) {
	arg_t ret = arg;

	char *b = arg_r_chr(arg, '/');
	if (b) {
		size_t new_l = arg.len - (b - arg.ptr) - (b == arg.ptr ? 0 : 1);
		size_t new_s = arg.asz - new_l;
		ret.len = new_l;
		ret.asz = new_s;
		ret.ptr = b + 1;
	}

	return ret;
}
