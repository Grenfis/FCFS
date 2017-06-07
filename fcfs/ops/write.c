#include "common.h"

int
ops_write(const char *path, const char *data, size_t size, off_t offset, struct fuse_file_info *fi) {
    DEBUG("size %lu", size);
    DEBUG("offset %lu", offset);
    if(fi != NULL) {
        DEBUG("fid %lu", fi->fh);
        DEBUG("file size %d", fi->flags);
    }

    return size;
}
