#include "common.h"
#include "../cache.h"

int
ops_releasedir(const char *path, struct fuse_file_info *fi)
{
    DEBUG();
    cache_seq_rm(fi->fh);

    return 0;
}
