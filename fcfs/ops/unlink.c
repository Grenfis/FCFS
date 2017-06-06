#include "common.h"

int
ops_unlink(const char *path) {
    DEBUG();

    fcfs_args_t *args = fcfs_get_args();
    int fid = fcfs_get_fid(path);
    int pfid = fcfs_get_pfid(path);

    dev_rm_file(args, fid, pfid);

    return 0;
}
