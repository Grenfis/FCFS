#include "fcfs_opts.h"

#include <stdio.h>
#include <fuse3/fuse.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>

#include "debug.h"
#include "fcfs_mount.h"
#include "utils.h"

static fcfs_args_t *
fcfs_get_args(void) {
    return (fcfs_args_t*)fuse_get_context()->private_data;
}

static int
fcfs_get_fid(const char *path) {
    struct stat st;
    fcfs_getattr(path, &st, NULL);
    return st.st_dev;
}

static int
fcfs_get_pfid(const char *path) {
    int p_len = get_parrent_path(path);
    p_len = p_len == 0 ? 1: p_len;
    char *np = calloc(1, p_len);
    memcpy(np, path, p_len);
    int res = fcfs_get_fid(np);
    free(np);
    return res;
}

void *
fcfs_init(struct fuse_conn_info *conn, struct fuse_config *cfg) {
    DEBUG("");
    fcfs_args_t *args = fcfs_get_args();
    args->dev = fopen(args->p_dev, "w+b");

    int res = fcfs_mount(args);
    if(res != 0) {
        ERROR("when mounting");
        exit(-1);
    }

    return (void*)args;
}

void
fcfs_destroy(void *a) {
    DEBUG("");
    fcfs_args_t *args = (fcfs_args_t*)a;
    free(args->p_dev);
    DEBUG("deleted p_dev");
    free(args->passwd);
    DEBUG("deleted passwd");
    free(args->fs_head);
    DEBUG("deleted fs_head");
    free(args->fs_bitmap);
    DEBUG("deleted fs_bitmap");
    free(args->fs_table);
    DEBUG("deleted fs_table");
    fclose(args->dev);
    DEBUG("deleted dev");
}

int
fcfs_getattr(const char *path, struct stat *stbufm, struct fuse_file_info *fi) {
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
        stbufm->st_dev = 0; //file id
        stbufm->st_mode = 0040000 | 0777; //todo: permission system
        stbufm->st_nlink = 2;

        //fcfs_getattr_cache.fid = 0;
        //strcpy(fcfs_getattr_cache.path, path);

        return 0;
    }
    //get directory content
    int dirs_len = 0;
    fcfs_dir_entry_t *dirs = fcfs_read_directory(args, fcfs_get_pfid(path), &dirs_len);
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
            stbufm->st_atime = dirs[i].access_date;
            stbufm->st_mtime = dirs[i].change_date;
            stbufm->st_ctime = dirs[i].create_date;
            stbufm->st_nlink = args->fs_table->entrys[dirs[i].file_id].link_count;
            stbufm->st_mode =  dirs[i].mode;
            stbufm->st_size = fcfs_get_file_size(args, dirs[i].file_id);
            stbufm->st_dev = dirs[i].file_id;

            free(dirs);
            return 0;
        }
    }
    //if smth was bad
    free(dirs);
    return -ENOENT;
}

int
fcfs_readdir(   const char *path,
                void *buf,
                fuse_fill_dir_t filler,
                off_t offset,
                struct fuse_file_info *fi,
                enum fuse_readdir_flags flags) {
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
    fcfs_dir_entry_t *dirs = fcfs_read_directory(args, fid, &dirs_len);
    if(dirs == NULL || dirs_len == 0) {
        if(dirs != NULL)
            free(dirs);
        return 0;
    }

    for(size_t i = 0; i < dirs_len; ++i) {
        //if(args->fs_table->entrys[dirs[i].file_id].link_count != 0)
        if(strlen(dirs[i].name) != 0) //if entry is not deleted
            filler(buf, dirs[i].name, NULL, 0, 0);
    }

    free(dirs);
    return 0;
}

int
fcfs_mkdir(const char *path, mode_t mode) {
    DEBUG("path = %s", path);
    DEBUG("mode = %d", mode);
    fcfs_args_t *args = fcfs_get_args();
    //get directory content
    int dirs_len = 0;
    int pfid = fcfs_get_pfid(path);
    fcfs_dir_entry_t *dirs = fcfs_read_directory(args, pfid, &dirs_len);

    fcfs_dir_entry_t *tmp = calloc(1, sizeof(fcfs_dir_entry_t) * (dirs_len + 1));
    memcpy(tmp, dirs, sizeof(fcfs_dir_entry_t) * dirs_len);
    free(dirs);

    int new_fid = fcfs_get_free_fid(args);
    if(new_fid <= 0) {
        return -ENOENT;
    }

    fcfs_alloc(args, new_fid);
    //fcfs_init_dir(args, new_fid);
    tmp[dirs_len].file_id       = new_fid; //set free file id
    tmp[dirs_len].mode          = 0040000 | mode;
    tmp[dirs_len].create_date   = time(NULL);
    tmp[dirs_len].access_date   = time(NULL);
    tmp[dirs_len].change_date   = time(NULL);

    int p_len = get_parrent_path(path);
    p_len += 1;
    strcpy(tmp[dirs_len].name, path + p_len);

    DEBUG("old dir len = %d", dirs_len);
    DEBUG("new dir len = %d", dirs_len + 1);
    DEBUG("name        = %s", path + p_len);

    fcfs_write_directory(args, pfid, tmp, dirs_len + 1);

    return 0;
}

int
fcfs_rmdir(const char *path) {
    DEBUG("path %s", path);

    fcfs_args_t *args = fcfs_get_args();
    int fid = fcfs_get_fid(path);
    fcfs_remove_file(args, fid);
    fcfs_remove_from_dir(args, fcfs_get_pfid(path), fid);
    
    return 0;
}

int
fcfs_open(const char *path, struct fuse_file_info *fi) {
    DEBUG("");
    return 0;
}

int
fcfs_read(  const char *path,
            char *buf, size_t size,
            off_t offset,
            struct fuse_file_info *fi) {
    DEBUG("");
    return 0;
}
