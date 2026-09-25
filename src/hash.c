#include <stdlib.h>
#include <string.h>

#include "hash.h"
#include "arg.h"

uint64_t hash_arg(const arg_t src) {
	if (!src.ptr) return 0;
	uint64_t ret = 0;

	#if HASH_TYPE == FNV_1 || HASH_TYPE == FNV_1A
	ret = FNV_PRIME;
	#endif

	#if HASH_TYPE == FNV_1
	for (size_t i = 0; i < src.len; i++) {
		ret *= FNV_PRIME;
		ret ^= src.ptr[i];
	}

	#elif HASH_TYPE == FNV_1A
	for (size_t i = 0; i < src.len; i++) {
		ret ^= src.ptr[i];
		ret *= FNV_PRIME;
	}

	#endif // HASH_TYPE comps

	return ret;
}

// return NULL on error, that being when appending, there's already an entry
// in that index, so the addition has the index be incremented until it finds
// an unused entry, and if an unused entry isn't found (bumped into a different
// hashed item, as in, different hash), that's the error
hash_pair *hash_map_put(const hash_map *map, const arg_t str) {
	if (!map) return NULL;
	uint64_t hash = hash_arg(arg_basename(str));
	size_t i = hash % map->icnt;

	while (map->list[i].arg.ptr
		&& map->list[i].hash == hash
		&& arg_cmp(map->list[i].arg, str)) {
			hash++;
			i = hash % map->icnt;
	}

	if (map->list[i].arg.ptr) return NULL;
	else {
		map->list[i].hash = hash;
		map->list[i].arg  = str;
	}

	return &map->list[i];
}

hash_map make_hash_map(const arg_arr_t list, const hash_flag flag) {
	// plus 1 was seemingly required
	hash_map ret = {
		.list = calloc(sizeof(ret.list[0]), list.icnt + 1),
		.icnt = list.icnt + 1,
		.flag = flag,
	};

	if (ret.flag == INIT_ON_START && ret.list) {
		for (size_t i = 0; i < list.icnt; i++) {
			hash_map_put(&ret, list.ptr[i]);
		}
	}

	return ret;
}

arg_t hash_map_fetch(hash_map *map, const arg_t str) {
	arg_t ret = {0};
	if (!map || !map->icnt) return ret;

	uint64_t hash = hash_arg(str);
	size_t i = hash % map->icnt;

	while (arg_cmp(str, arg_basename(map->list[i].arg))) {
		if (!map->list[i].arg.ptr
			|| map->list[i].hash != hash) return ret;

		hash++;
		i = hash % map->icnt;
	}

	ret = map->list[i].arg;
	return ret;
}

void free_hash_map(hash_map *map) {
	if (!map) return;

	if (map->list) free(map->list);
	map->list = NULL;
	map->icnt = 0;
}
