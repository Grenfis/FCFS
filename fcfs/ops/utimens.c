#include "common.h"

int
ops_utimens(const char *path, const struct timespec tv[2], struct fuse_file_info *fi)
{
    DEBUG();
    return 0;
}
