#include "common.h"

int
ops_open(const char *path, struct fuse_file_info *fi) {
    DEBUG("path %s", path);
    fi->fh = fcfs_get_fid(path);

    DEBUG("fid %lu", fi->fh);

    return 0;
}
