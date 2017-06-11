#ifndef CACHE_H
#define CACHE_H

#include <sys/stat.h>

enum {
    CACHE_FID_MAX = 256,
};

/*typedef struct cache_fid_item {
    char                    *path;
    //int                     fid;
    struct stat             stat;
    struct cache_fid_item   *next;
} cache_fid_item_t;

typedef struct cache_fid_list {
    cache_fid_item_t    *first;
    int                 cnt;
} cache_fid_list_t;*/

void
cache_init();

void
cache_destroy();

void
cache_fid_add(const char *path, struct stat *st);

struct stat *
cache_fid_get(const char *path);

#endif
