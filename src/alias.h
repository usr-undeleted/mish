#ifndef ALIAS_H
#define ALIAS_H

#include "arg.h"

// relate an arg to an alias
// return the name field itself and
// set the status to non-zero if failed
arg_t find_alias(const arg_t name, int *status);

// add a new alias, where 'label' is the argument that
// would get replaced by 'cmd'
int new_alias(const arg_t label, const arg_t cmd);

// remove an alias
int remove_alias(const arg_t label);

#endif
