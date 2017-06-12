#ifndef CACHE_H
#define CACHE_H

#include <sys/stat.h>

void
cache_init();

void
cache_destroy();

void
cache_fid_init();

void
cache_fid_destroy();

void
cache_fid_add(const char *path, struct stat *st);

struct stat *
cache_fid_get(const char *path);

#endif
