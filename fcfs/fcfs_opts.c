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

static fcfs_getattr_bentry_t fcfs_getattr_cache = {
    .path   = NULL,
    .fid    = -1
};

static fcfs_args_t *
fcfs_get_args(void) {
    return (fcfs_args_t*)fuse_get_context()->private_data;
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

    fcfs_getattr_cache.path = calloc(1, FCFS_MAX_FILE_NAME_LENGTH);
    return (void*)args;
}

void
fcfs_destroy(void *a) {
    DEBUG("");
    fcfs_args_t *args = (fcfs_args_t*)a;
    free(args->p_dev);
    free(args->passwd);
    free(args->fs_head);
    free(args->fs_bitmap);
    free(args->fs_table);
    fclose(args->dev);

    free(fcfs_getattr_cache.path);
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
        stbufm->st_mode = 0040000 | 0777; //todo: permission system
        stbufm->st_nlink = 2;

        fcfs_getattr_cache.fid = 0;
        strcpy(fcfs_getattr_cache.path, path);

        return 0;
    }
    //if requested path is not match with old path
    //then update path in cache
    if(!pathcmp(fcfs_getattr_cache.path, path)) {
        strcpy(fcfs_getattr_cache.path, "/");
        fcfs_getattr_cache.fid = 0;
        DEBUG("cache updated");
    }
    //get directory content
    int dirs_len = 0;
    fcfs_dir_entry_t *dirs = fcfs_read_directory(args, fcfs_getattr_cache.fid, &dirs_len);
    if(dirs == NULL || dirs_len == 0) {
        if(dirs != NULL)
            free(dirs);
        return -ENOENT;
    }

    int p_len = strlen(fcfs_getattr_cache.path);
    for(size_t i = 0; i < dirs_len; ++i) {
        //if directory contents requested file
        if(strcmp(dirs[i].name, path + p_len) == 0) {
            stbufm->st_atime = dirs[i].access_date;
            stbufm->st_mtime = dirs[i].change_date;
            stbufm->st_ctime = dirs[i].create_date;

            fcfs_getattr_cache.fid = dirs[i].file_id;;
            strcpy(fcfs_getattr_cache.path, path);

            stbufm->st_nlink = args->fs_table->entrys[dirs[i].file_id].link_count;
            stbufm->st_mode = dirs[i].mode; //todo: permission system

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
    fcfs_dir_entry_t *dirs = fcfs_read_directory(args, fcfs_getattr_cache.fid, &dirs_len);
    if(dirs == NULL || dirs_len == 0) {
        if(dirs != NULL)
            free(dirs);
        return 0;
    }

    for(size_t i = 0; i < dirs_len; ++i) {
        struct stat *st = calloc(1, sizeof(struct stat));
        st->st_uid      = getuid();
        st->st_gid      = getgid();
        st->st_atime    = dirs[i].access_date;
        st->st_mtime    = dirs[i].change_date;
        st->st_ctime    = dirs[i].create_date;
        st->st_nlink    = args->fs_table->entrys[dirs[i].file_id].link_count;
        st->st_mode     = dirs[i].mode; //todo: permission system
        filler(buf, dirs[i].name, st, 0, 0);
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
    fcfs_dir_entry_t *dirs = fcfs_read_directory(args, fcfs_getattr_cache.fid, &dirs_len);

    fcfs_dir_entry_t *tmp = calloc(1, sizeof(fcfs_dir_entry_t) * (dirs_len + 1));
    memcpy(tmp, dirs, sizeof(fcfs_dir_entry_t) * dirs_len);
    free(dirs);

    int p_len = strlen(fcfs_getattr_cache.path);

    tmp[dirs_len].file_id       = 0; //set free file id
    tmp[dirs_len].mode          = mode;
    tmp[dirs_len].create_date   = time(NULL);
    tmp[dirs_len].access_date   = time(NULL);
    tmp[dirs_len].change_date   = time(NULL);
    strcpy(tmp[dirs_len + 1].name, path + p_len);

    DEBUG("old dir len = %d", dirs_len);
    DEBUG("new dir len = %d", dirs_len + 1);

    return 0;
}

int
fcfs_rmdir(const char *path) {
    DEBUG("");
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
