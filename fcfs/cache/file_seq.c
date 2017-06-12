#include "../cache.h"
#include "../hashmap/hashmap.h"

#include <stdlib.h>
#include <string.h>

map_t map_seq;

static int
seq_destroy(any_t a, any_t b)
{
    free(b);
    return MAP_OK;
}

void
cache_seq_init()
{
    map_seq = hashmap_new();
}

void
cache_seq_destroy()
{
    //hashmap_iterate(map_seq, seq_destroy, NULL);
    hashmap_clear(map_seq, seq_destroy, NULL);
    hashmap_free(map_seq);
}

void
cache_seq_add(const char *path, dev_blk_info_t *l)
{

}

dev_blk_info_t *
cache_seq_get(const char *path)
{

}
