#ifndef PATH_H
#define PATH_H

#include "arg.h"

// use to get the stuff from $PATH
void refresh_path(void);

// get a binary path from a binary name
char *fetch_from_path(const arg_t bin);

#endif
