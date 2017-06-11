#include "cache.h"
#include "common.h"
#include "../hashmap/hashmap.h"

#include <time.h>

typedef struct c_fid_item {
    struct stat st;
    //char        *path;
    time_t      time;
} c_fid_item_t;

static map_t    map_fid;

static int
fid_destroy(any_t a, any_t b) {
    //free(((c_fid_item_t*)b)->path);
    free(b);
    return MAP_OK;
}

static void
cache_fid_init()
{
    map_fid = hashmap_new();
}

static void
cache_fid_destroy()
{
    hashmap_iterate(map_fid, fid_destroy, NULL);
    hashmap_free(map_fid);
}

void
cache_fid_add(const char *path, struct stat *st)
{
    c_fid_item_t *it = calloc(1, sizeof(c_fid_item_t));
    it->time = time(NULL);
    //it->path = calloc(1, strlen(path));
    //strcpy(it->path, path);
    memcpy(&it->st, st, sizeof(struct stat));
    int error = hashmap_put(map_fid, path, it);
    if(error == MAP_FULL || error == MAP_OMEM)
    {

    }
}

struct stat *
cache_fid_get(const char *path)
{
    struct stat *st= calloc(1, sizeof(struct stat));
    c_fid_item_t *it;
    int error = hashmap_get(map_fid, path,(void**)&it);
    if(error != MAP_OK)
        return NULL;
    memcpy(st, &it->st, sizeof(struct stat));
    return st;
}

void
cache_init()
{
    cache_fid_init();
}

void
cache_destroy()
{
    cache_fid_destroy();
}
