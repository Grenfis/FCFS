#include "common.h"

int
ops_readdir(   const char *path,
                void *buf,
                fuse_fill_dir_t filler,
                off_t offset,
                struct fuse_file_info *fi,
                enum fuse_readdir_flags flags)
{
    DEBUG("path     = %s", path);
    DEBUG("offset   = %ld", offset);
    DEBUG("flags    = %d", flags);
    fcfs_args_t *args = fcfs_get_args();
    //for all dirs
    filler(buf, ".", NULL, 0, 0);
    filler(buf, "..", NULL, 0, 0);
    //get directory content
    int dirs_len = 0;
    int fid = fcfs_get_fid(path);
    fcfs_dir_entry_t *dirs = dev_read_dir(args, fid, &dirs_len);
    if(dirs == NULL || dirs_len == 0)
    {
        if(dirs != NULL)
            free(dirs);
        return 0;
    }

    for(size_t i = 0; i < dirs_len; ++i)
    {
        if(strlen(dirs[i].name) != 0) //if entry is not deleted
            filler(buf, dirs[i].name, NULL, 0, 0);
    }

    if(dirs != NULL)
        free(dirs);
    return 0;
}
