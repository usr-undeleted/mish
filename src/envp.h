#ifndef ENVP_H
#define ENVP_H

#include "arg.h"

// export keyword will add custom keywords here
//
// i've managed to make a good explanation on
// shell_envp's comments, so i suppose any contributor
// should check that variable (on the correct translation file)
extern dbl_ptr_t child_envp;

// produce child_envp, making sure to add a null term
//
// doesn't strdup envp, so never free stuff from here
bool make_child_envp(const char **envp);

// makes a new entry on the shell_envp
bool shell_set_env(const char *label, const char *val);

// move an env var from the shell context to the
// child context
//
// if val already exists:
// val is NULL: copy it over
// val is non-zero: set env and copy it over
bool export_env(const char *label, const char *val);

// remove an environment variable from all contexts,
// for example, freeing the memory from shell_envp, and
// removing it from child_envp too
bool unset_env(const char *label);

// get an env, used by the shell
char *shell_get_env(const char *label);

#endif
