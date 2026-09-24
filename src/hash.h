#ifndef HASH_H
#define HASH_H

#include <stdint.h>

#include "arg.h"

// all hash map fetches/sets are to be done with basename
//
// not really good, yes, but i don't want to add a bool just for a
// basename usage toggle (atleast not now)

// all hash maps are expected to be initiated to zero

#if HASH_TYPE != FNV_1 && HASH_TYPE != FNV_1A
#error "this hash type is either unimplemented or unknown"
#endif

// fnv hash definitions
// https://www.isthe.com/chongo/tech/comp/fnv/
#define FNV_PRIME  1099511628211UL
#define FNV_OFFSET 14695981039346656037UL

// hash an arg to a 64 bit value
uint64_t hash_arg(const arg_t src);

// pointer would ideally not be allocated
typedef struct {
	arg_t     arg;
	uint64_t hash;

} hash_pair;

typedef enum {
	INIT_ON_START, // hash everything at the start
	NO_INIT,       // won't hash automatically at init (do it yerself, im lazy!)

} hash_flag;

// non-used entries will have a null pointer
typedef struct {
	hash_pair *list;
	hash_flag  flag;
	size_t     icnt;
} hash_map;

// make an allocated hash table
//
// hash map will contain a NULL array on failure
hash_map make_hash_map(const arg_arr_t list, const hash_flag flag);

// fetch a string from a hash map
//
// return NULL on failure
//
// will add to the hash map if the flag allows for it
arg_t hash_map_fetch(hash_map *map, const arg_t str);

void free_hash_map(hash_map *map);

// return NULL on error, that being when appending, there's already an entry
// in that index, so the addition has the index be incremented until it finds
// an unused entry, and if an unused entry isn't found (bumped into a different
// hashed item, as in, different hash), that's the error
hash_pair *hash_map_put(const hash_map *map, const arg_t str);

#endif
