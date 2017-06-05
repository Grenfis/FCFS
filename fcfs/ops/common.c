#include "../ops.h"
#include "../utils.h"
#include "../fcfs.h"
#include "common.h"

#include <stdlib.h>
#include <string.h>
#include <debug.h>

fcfs_args_t *
fcfs_get_args(void) {
    return (fcfs_args_t*)fuse_get_context()->private_data;
}

int
fcfs_get_fid(const char *path) {
    struct stat st;
    ops_getattr(path, &st, NULL);
    return st.st_dev;
}

int
fcfs_get_pfid(const char *path) {
    int p_len = get_parrent_path(path);
    p_len = p_len == 0 ? 1: p_len;
    //allocate tmp string
    char *np = calloc(1, p_len);
    memcpy(np, path, p_len);
    //get file id by path
    int res = fcfs_get_fid(np);

    free(np);
    return res;
}
