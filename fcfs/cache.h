#ifndef CACHE_H
#define CACHE_H

#include <sys/stat.h>
#include "dev.h"

void
cache_init();

void
cache_destroy();

void
cache_fid_init();

void
cache_fid_destroy();

void
cache_seq_init();

void
cache_seq_destroy();

void
cache_fid_add(const char *path, struct stat *st);

struct stat *
cache_fid_get(const char *path);

void
cache_seq_add(const char *path, dev_blk_info_t *l);

dev_blk_info_t *
cache_seq_get(const char *path);

#endif
