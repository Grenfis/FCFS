#include "../cache.h"
#include "../../hashmap/hashmap.h"

#include <stdlib.h>
#include <string.h>

map_t map_seq;

static int
seq_len(dev_blk_info_t *inf)
{
    int i = 0;
    while(inf != NULL)
    {
        i++;
        inf = inf->next;
    }
    return i;
}

static int
seq_destroy(any_t a, any_t b)
{
    dev_destr_blk_info(b);
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
    hashmap_clear(map_seq, seq_destroy, NULL);
    hashmap_free(map_seq);
}

void
cache_seq_add(int id, dev_blk_info_t *l)
{
    char p[255];
    sprintf(p, "%d", id);
    int error = hashmap_put(map_seq, p, l);
    if(error == MAP_FULL || error == MAP_OMEM)
    {
        hashmap_clear(map_seq, seq_destroy, NULL);
    }
}

dev_blk_info_t *
cache_seq_get(int id, int *sz)
{
    char p[255];
    sprintf(p, "%d", id);
    dev_blk_info_t *t;
    int error = hashmap_get(map_seq, p, (void**)&t);
    if(error != MAP_OK)
        return NULL;
    *sz = seq_len(t);
    return t;
}

void
cache_seq_rm(int id)
{
    char p[255];
    sprintf(p, "%d", id);

    int sz = 0;
    dev_blk_info_t *inf = cache_seq_get(id, &sz);
    if(inf == NULL)
        return;
    dev_destr_blk_info(inf);
    hashmap_remove(map_seq, p);
}
