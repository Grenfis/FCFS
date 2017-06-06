#include "common.h"

int
ops_create( const char *path, mode_t mode, struct fuse_file_info *fi) {
    DEBUG();
    fcfs_args_t *args = fcfs_get_args();
    int pfid = fcfs_get_pfid(path);
    int fid  = dev_free_fid(args);

    int p_len = get_parrent_path(path);
    p_len += 1;

    dev_create_file(args, pfid, fid, path + p_len, 0100000 | mode);

    return 0;
}
