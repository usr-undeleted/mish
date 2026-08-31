#ifndef ALIAS_H
#define ALIAS_H

#include "arg.h"

// relate an arg to an alias
//
// return the name field itself and
// set the status to non-zero if failed
arg_t find_alias(const arg_t name, int *status);

// add a new alias, where 'label' is the argument that
// would get replaced by 'cmd'
int new_alias(const arg_t label, const arg_t cmd);

// remove an alias
int remove_alias(const arg_t label);

// with an index shared beetwen the function and caller,
// reveal contents from the alias list, returning NULL if
// the index doesn't contain anything
arg_t *reveal_alias(size_t *i);

#endif
