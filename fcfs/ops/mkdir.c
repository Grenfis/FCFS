#include "common.h"

int
ops_mkdir(const char *path, mode_t mode)
{
    DEBUG("path = %s", path);
    DEBUG("mode = %d", mode);
    fcfs_args_t *args = fcfs_get_args();
    //get directory content
    int p_len = get_parrent_path(path);
    p_len += 1;

    int fid = dev_free_fid(args);
    dev_create_file(args, fcfs_get_pfid(path), fid, path + p_len, 0040000 | mode);
    dev_init_file(args, fid);
    return 0;
}
