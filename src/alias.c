#include "alias.h"
#include "arg.h"

#define ALLOC_SZ 8

// contains all aliases as 'label=cmd'
//
// deleted args will have asz set to zero, due
// to how free_arg naturally works
arg_arr_t aliases = {0};

// find an index inside of the list
// this can be either a freed arg (asz = 0) or
// the last item in the list
size_t find_list_idx(void) {
	size_t ret = 0;

	while (ret < aliases.icnt && aliases.ptr[ret].asz != 0) ret++;

	return ret;
}

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
	// see if that alias wasn't taken already
	arg_t *ptr =  alias_exists(label);

	if (!ptr) {
		// alias doesn't exist yet
		size_t i = find_list_idx();

		// if the idx is at the end of the list, and we would need more allocation
		if (i == aliases.icnt && (aliases.icnt + 1) >= (aliases.asz / sizeof(arg_t))) {
			if (alloc_arg_arr(&aliases, ALLOC_SZ)) return 1;
		}

		ptr = &aliases.ptr[aliases.icnt];
		free_arg(ptr);
		if (append_arg(ptr, label)) return 1;
		if (append_arg(ptr, make_arg("="))) return 1;
		if (append_arg(ptr, cmd)) return 1;

		aliases.icnt++;

	} else {
		// alias is in middle of list, replace it
		zero_arg(ptr);
		if (append_arg(ptr, label)) return 1;
		if (append_arg(ptr, make_arg("="))) return 1;
		if (append_arg(ptr, cmd)) return 1;

	}

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
