#include "common.h"
#include "../cache.h"

int
ops_opendir(const char *path, struct fuse_file_info *fi)
{
    DEBUG();
    fi->fh = fcfs_get_fid(path);

    fcfs_args_t *args = fcfs_get_args();
    int sz = 0;
    dev_blk_info_t *inf =  dev_get_file_seq(args, fi->fh, &sz);
    cache_seq_add(fi->fh, inf);

    return 0;
}
