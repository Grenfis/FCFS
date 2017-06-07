#include "common.h"

int
ops_flush(const char *path, struct fuse_file_info *fi) {
    DEBUG("path %s", path);
    if(fi != NULL) {
        DEBUG("fid %lu", fi->fh);
    }
    return 0;
}
