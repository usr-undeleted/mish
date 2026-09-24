#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <libgen.h>
#include <stdio.h>

#include "path.h"
#include "envp.h"
#include "hash.h"
#include "arg.h"

// how many items to allocate per (re)allocation
#define ALLOC_SZ 32

// allocated list of known paths
arg_arr_t known_paths = {0};
#define LIST_MAX_ITEMS (known_paths.asz / sizeof(arg_t))

// hash map of everything
hash_map map = {0};

// clear the list
void clear_list(void) {
    for (size_t i = 0; i < known_paths.icnt; i++) {
        if (!known_paths.ptr[i].ptr) continue;
        zero_arg(&known_paths.ptr[i]);
    }

    known_paths.icnt = 0;
    free_hash_map(&map);
}

// free the memory from the list
void free_path(void) {
    for (size_t i = 0; i < known_paths.icnt; i++) {
        if (!known_paths.ptr[i].ptr) continue;
        free(known_paths.ptr[i].ptr);
    }

    free(known_paths.ptr);
    free_hash_map(&map);
}

// append a new item to the list
void append_to_list(const char *path) {
    // since we use .icnt for the index...
    // not 'nuff space, must (re)allocate
    if (known_paths.icnt > (LIST_MAX_ITEMS - (LIST_MAX_ITEMS ? 1 : 0)) ||
        !known_paths.ptr) {
            if (alloc_arg_arr(&known_paths, ALLOC_SZ) != 0) return;
    }

    known_paths.ptr[known_paths.icnt].ptr = strdup(path);
    if (!known_paths.ptr[known_paths.icnt].ptr) return;
    size_t len = strlen(path);
    known_paths.ptr[known_paths.icnt].asz = len;
    known_paths.ptr[known_paths.icnt].len = len;

    known_paths.icnt++;

    free_hash_map(&map);
    map = make_hash_map(known_paths, NO_INIT);
}

// append to list every binary from received directory
void append_from_path(const char *path) {
    DIR *dir = opendir(path);
    if (!dir) return;

    struct dirent *file = NULL;
    while ((file = readdir(dir))) {
        if (file->d_type != DT_REG) continue;

        char file_path[PATH_MAX + 1] = {0};
        snprintf(file_path, sizeof(file_path) - 1, "%s/%s", path, file->d_name);
        struct stat file_st = {0};
        if (stat(file_path, &file_st) != 0) continue;

        if (file_st.st_mode & S_IEXEC) append_to_list(file_path);
    }
}

// read $PATH and figure out the paths for executable files
void populate_list(void) {
    char *env = shell_get_env("PATH");
    if (!env) return;

    // loop trough everything between colons
    char *path  = env;
    char *colon = strchr(path, ':');

    while (1) {
        if (colon) *colon = '\0';

        append_from_path(path);

        if (colon) *colon = ':';

        path = colon;
        if (path) ++path;
        else break;

        colon = strchr(path, ':');
    }
}

// use to get the stuff from $PATH
void refresh_path(void) {
    clear_list();
    populate_list();
    free_hash_map(&map);
    map = make_hash_map(known_paths, NO_INIT);
}

// get a binary path from a binary name
char *fetch_from_path(const arg_t bin) {
	// try hash map first
	bool put_on_hash = false;
	arg_t hash_attempt = hash_map_fetch(&map, bin);
	if (!hash_attempt.ptr) put_on_hash = true;
	else return hash_attempt.ptr;

	for (size_t i = 0; i < known_paths.icnt; i++) {
        if (!known_paths.ptr[i].ptr) continue;

        arg_t comp_arg = known_paths.ptr[i];
        comp_arg.ptr   = basename(comp_arg.ptr);
        comp_arg.asz   = 0;
        comp_arg.len  -= comp_arg.ptr - known_paths.ptr[i].ptr;

        if (!arg_cmp(comp_arg, bin)) {
        	// put on hash map since we failed last time
        	if (put_on_hash) hash_map_put(&map, known_paths.ptr[i]);
        	return known_paths.ptr[i].ptr;
        }
    }

    return NULL;
}

// return the contents of the path list from an index, while also
// editing that same index. returns NULL on end
char *reveal_path(size_t *i) {
	if (*i >= known_paths.icnt) {
		return NULL;

	} else {
		return known_paths.ptr[(*i)++].ptr;
	}
}
