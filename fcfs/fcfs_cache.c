#include "fcfs_cache.h"

#include <fcfs_structs.h>
#include <stdlib.h>
#include <string.h>
#include <debug.h>

fcfs_path_cache_t *
fcfs_pcache_create() {
    DEBUG();
    fcfs_path_cache_t *c = calloc(1, sizeof(fcfs_path_cache_t));
    for(size_t i = 0; i < FCFS_PATH_CACHE_SZ; ++i) {
        c->entrys[i].fid = 0;
        c->entrys[i].path = calloc(1, FCFS_MAX_FILE_NAME_LENGTH);
        strcpy(c->entrys[i].path, "/");
    }
    return c;
}

void
fcfs_pcache_destroy(fcfs_path_cache_t *c) {
    DEBUG();
    for(size_t i = 0; i < FCFS_PATH_CACHE_SZ; ++i) {
        free(c->entrys[i].path);
    }
    free(c);
}

void
fcfs_pcache_add(fcfs_path_cache_t *c, fcfs_getattr_bentry_t *e) {
    DEBUG();
    for(size_t i = 0; i < FCFS_PATH_CACHE_SZ; ++i) {
        if(strcmp(c->entrys[i].path, e->path) == 0)
            return;
    }
    c->entrys[c->pos].fid = e->fid;
    strcpy(c->entrys[c->pos].path, e->path);

    c->pos++;
    if(c->pos >= FCFS_PATH_CACHE_SZ)
        c->pos = 0;
}

void
fcfs_pcache_get(fcfs_path_cache_t *c, fcfs_getattr_bentry_t *e) {
    DEBUG();
    for(size_t i = 0; i < FCFS_PATH_CACHE_SZ; ++i) {
        if(strcmp(c->entrys[i].path, e->path) == 0) {
            e->fid = c->entrys[i].fid;
            return;
        }
    }
    strcpy(e->path, "/");
    e->fid = 0;
}
