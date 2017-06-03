#ifndef FCFS_CACHE_H
#define FCFS_CACHE_H

#define FCFS_PATH_CACHE_SZ 25

typedef struct fcfs_getattr_bentry {
    char *path;
    int fid;
} fcfs_getattr_bentry_t;

typedef struct fcfs_path_cache {
    int pos;
    fcfs_getattr_bentry_t entrys[FCFS_PATH_CACHE_SZ];
} fcfs_path_cache_t;

fcfs_path_cache_t *
fcfs_pcache_create();

void
fcfs_pcache_destroy(fcfs_path_cache_t *c);

void
fcfs_pcache_add(fcfs_path_cache_t *c, fcfs_getattr_bentry_t *e);

void
fcfs_pcache_get(fcfs_path_cache_t *c, fcfs_getattr_bentry_t *e);

#endif
