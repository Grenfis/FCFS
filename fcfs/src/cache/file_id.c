#include "../cache.h"
#include "../../hashmap/hashmap.h"

#include <stdlib.h>
#include <string.h>

static map_t map_fid;

static int
fid_destroy(any_t a, any_t b)
{
    if(b != NULL)
        free(b);
    return MAP_OK;
}

void
cache_stat_init()
{
    map_fid = hashmap_new();
}

void
cache_stat_destroy()
{
    //hashmap_iterate(map_fid, fid_destroy, NULL);
    hashmap_clear(map_fid, fid_destroy, NULL);
    hashmap_free(map_fid);
}

void
cache_stat_add(const char *path, struct stat *st)
{
    struct stat *tmp = malloc(sizeof(struct stat));
    memcpy(tmp, st, sizeof(struct stat));
    int error = hashmap_put(map_fid, path, tmp);
    if(error == MAP_FULL || error == MAP_OMEM)
    {
        hashmap_clear(map_fid, fid_destroy, NULL);
    }
}

struct stat *
cache_stat_get(const char *path)
{
    struct stat *st= calloc(1, sizeof(struct stat));
    struct stat *tmp;
    int error = hashmap_get(map_fid, path,(void**)&tmp);
    if(error != MAP_OK)
        return NULL;
    memcpy(st, tmp, sizeof(struct stat));
    return st;
}
