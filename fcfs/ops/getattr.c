#include "common.h"

int
ops_getattr(const char *path, struct stat *stbufm, struct fuse_file_info *fi) {
    DEBUG("path = %s", path);
    fcfs_args_t *args = fcfs_get_args();
    //set default data of file
    memset(stbufm, 0, sizeof(struct stat));
    stbufm->st_uid = getuid();
    stbufm->st_gid = getgid();
    stbufm->st_atime = time(NULL);
    stbufm->st_mtime = time(NULL);
    //if it's root return constant data
    if(strcmp(path, "/") == 0) {
        stbufm->st_dev      = 0; //file id
        stbufm->st_mode     = 0040000 | 0777; //todo: permission system
        stbufm->st_nlink    = 2;
        stbufm->st_size     = dev_file_size(args, 0);
        return 0;
    }
    //get directory content
    int dirs_len = 0;
    fcfs_dir_entry_t *dirs = dev_read_dir(args, fcfs_get_pfid(path), &dirs_len);
    if(dirs == NULL || dirs_len == 0) {
        if(dirs != NULL)
            free(dirs);
        return -ENOENT;
    }

    int p_len = get_parrent_path(path);
    p_len += 1;

    for(size_t i = 0; i < dirs_len; ++i) {
        //if directory contents requested file
        if(strcmp(dirs[i].name, path + p_len) == 0) {
            stbufm->st_atime = dirs[i].atime;
            stbufm->st_mtime = dirs[i].mtime;
            stbufm->st_ctime = dirs[i].ctime;
            stbufm->st_nlink = args->fs_table->entrs[dirs[i].fid].lnk_cnt;
            stbufm->st_mode =  dirs[i].mode;
            stbufm->st_size = dev_file_size(args, dirs[i].fid);
            stbufm->st_dev = dirs[i].fid;

            free(dirs);
            return 0;
        }
    }
    //if smth was bad
    free(dirs);
    return -ENOENT;
}
