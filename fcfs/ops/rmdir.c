#include "common.h"

int
ops_rmdir(const char *path) {
    DEBUG("path %s", path);

    fcfs_args_t *args = fcfs_get_args();
    int fid = fcfs_get_fid(path);
    dev_rm_file(args, fid);
    dev_rm_from_dir(args, fcfs_get_pfid(path), fid);

    return 0;
}
