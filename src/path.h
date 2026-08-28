#ifndef PATH_H
#define PATH_H

#include "arg.h"

// use to get the stuff from $PATH
void refresh_path(void);

// get a binary path from a binary name
char *fetch_from_path(const arg_t bin);

// free the memory from the list
void free_path(void);

// return the contents of the path list from an index, while also
// editing that same index. returns NULL on end
char *reveal_path(size_t *i);

#endif
