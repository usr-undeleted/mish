#include "alias.h"
#include "arg.h"

#define ALLOC_SZ 8

// contains all aliases as 'label=cmd'
//
// deleted args will have asz set to zero, due
// to how free_arg naturally works
arg_arr_t aliases = {0};

// find an alias and return the pointer
arg_t *alias_exists(const arg_t name) {
	for (size_t i = 0; i < aliases.icnt; i++) {
		// exclude freed stuff
		if (!aliases.ptr[i].asz) continue;

		arg_t comp = cap_to_char(aliases.ptr[i], '=');

		if (!arg_cmp(name, comp)) {
			return &aliases.ptr[i];
		}
	}

	return NULL;
}

arg_t find_alias(const arg_t name, int *status) {
	if (status) *status = 0;

	arg_t *p = alias_exists(name);
	if (p) return shift_arg(shift_arg_c(*p, '='), 1);

	if (status) *status = 1;
	return name;
}

int new_alias(const arg_t label, const arg_t cmd) {
	// make the full alias
	arg_t tmp = {0};
	if (append_arg(&tmp, label) != 0) return 1;
	if (append_arg(&tmp, make_arg("=")) != 0) return 1;
	if (append_arg(&tmp, cmd) != 0) return 1;

	if (append_arg_to_arr(&aliases, tmp, ALLOC_SZ)) return 1;

	free_arg(&tmp);
	return 0;
}

// remove an alias
int remove_alias(const arg_t label) {
	arg_t *ptr = alias_exists(label);
	if (!ptr) return 1;

	// is the last item on the list
	if (ptr == &aliases.ptr[aliases.icnt - 1]) aliases.icnt--;

	free_arg(ptr);
	ptr->asz = 0;
	return 0;
}

// free all alias args
void free_aliases(void) {
	for (size_t i = 0; i < aliases.icnt; i++) {
		if (!aliases.ptr[i].ptr) continue;
		free_arg(&aliases.ptr[i]);
	}
}

// with an index shared beetwen the function and caller,
// reveal contents from the alias list, returning NULL if
// the index goes beyond the list size. this does mean that
// it can return an empty item, so check for that!
arg_t *reveal_alias(size_t *i) {
	if (*i >= aliases.icnt) return NULL;

	return &aliases.ptr[*i];
}
