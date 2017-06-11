#include "cache.h"
#include "common.h"

static cache_fid_list_t cache_fid;

static void
cache_fid_init()
{
    cache_fid.cnt = 0;
    cache_fid_item_t *tmp = calloc(1, sizeof(cache_fid_item_t));
    cache_fid.first = tmp;
    for(size_t i = 0; i < CACHE_FID_MAX - 1; ++i) {
        tmp->path   = "";
        tmp->next   = calloc(1, sizeof(cache_fid_item_t));
        tmp         = tmp->next;
    }
}

static void
cache_fid_destroy()
{
    cache_fid_item_t *tmp = cache_fid.first;
    for(size_t i = 0; i < CACHE_FID_MAX; ++i) {
        if(tmp != NULL) {
            free(tmp->path);
            cache_fid_item_t *t = tmp->next;
            free(tmp);
            tmp = t;
        }else{
            break;
        }
    }
}

void
cache_fid_add(const char *path, struct stat *st)
{
    cache_fid_item_t *tmp = cache_fid.first;
    unsigned char flag = 0;
    for(size_t i = 0; i < cache_fid.cnt; ++i) {
        if(tmp == NULL)
            return;
        if(tmp->path == NULL)
            break;
        if(strcmp(path, tmp->path) == 0) {
            flag = 1;
            break;
        }
        tmp = tmp->next;
    }
    if(!flag) {
        if(strcmp(tmp->path, "") == 0) {
            tmp->path = calloc(1, strlen(path));
        }
        strcpy(tmp->path, path);
        //tmp->fid = fid;
        memcpy(&tmp->stat, st, sizeof(struct stat));
        cache_fid.cnt++;
    }
    if(cache_fid.cnt == CACHE_FID_MAX)
        cache_fid.cnt = 0;
}

struct stat *
cache_fid_get(const char *path)
{
    cache_fid_item_t *tmp = cache_fid.first;
    while(tmp != NULL) {
        if(tmp->path == NULL) {
            tmp = tmp->next;
            continue;
        }
        if(strcmp(path, tmp->path) == 0){
            return &tmp->stat;
        }
        tmp = tmp->next;
    }
    return NULL;
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
