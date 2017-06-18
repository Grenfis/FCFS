#include "common.h"
#include "../cache.h"

int
ops_open(const char *path, struct fuse_file_info *fi)
{
    DEBUG("path %s", path);
    fi->fh = fcfs_get_fid(path);

    DEBUG("fid %lu", fi->fh);
    /*fcfs_args_t *args = fcfs_get_args();
    int sz = 0;
    dev_blk_info_t *inf =  dev_get_file_seq(args, fi->fh, &sz);
    cache_seq_add(fi->fh, inf);*/

    return 0;
}
