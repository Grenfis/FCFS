#ifndef COMMON_H
#define COMMON_H

#include "../ops.h"
#include "../fcfs.h"
#include "../dev.h"

#include <utils.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <fuse3/fuse.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#include <debug.h>

typedef struct fcfs_args fcfs_args_t;

//get main fs data
static inline fcfs_args_t *
fcfs_get_args(void) {
    return (fcfs_args_t*)fuse_get_context()->private_data;
}

//get file if byt path
static inline int
fcfs_get_fid(const char *path) {
    struct stat st;
    _ops_getattr(path, &st, NULL, 0);
    return st.st_dev;
}

//get parent file id by path
static inline int
fcfs_get_pfid(const char *path) {
    int p_len = get_parrent_path(path);
    p_len = p_len == 0 ? 1: p_len;
    //allocate tmp string
    char *np = malloc(p_len + 1);
    memcpy(np, path, p_len);
    np[p_len] = '\0';
    //get file id by path
    int res = fcfs_get_fid(np);

    if(np != NULL)
        free(np);
    return res;
}


#endif
