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
#include "fcfs_cache.h"

static fcfs_path_cache_t *pcache = NULL;

static fcfs_getattr_bentry_t fcfs_getattr_cache = {
    .path   = NULL,
    .fid    = 0
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
    strcpy(fcfs_getattr_cache.path, "/");

    pcache = fcfs_pcache_create();
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
    fcfs_pcache_destroy(pcache);
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
        //delete last part of path
        int bl = get_parrent_path(path);
        bl = bl == 0 ? 1: bl;
        memcpy(fcfs_getattr_cache.path, path, bl);
        fcfs_getattr_cache.path[bl] = '\0';
        //check file path in cache
        fcfs_pcache_get(pcache, &fcfs_getattr_cache);
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

    int p_len = get_parrent_path(path);
    p_len += 1;

    for(size_t i = 0; i < dirs_len; ++i) {
        //if directory contents requested file
        if(strcmp(dirs[i].name, path + p_len) == 0) {
            stbufm->st_atime = dirs[i].access_date;
            stbufm->st_mtime = dirs[i].change_date;
            stbufm->st_ctime = dirs[i].create_date;

            fcfs_getattr_cache.fid = dirs[i].file_id;
            strcpy(fcfs_getattr_cache.path, path);
            fcfs_pcache_add(pcache, &fcfs_getattr_cache);

            stbufm->st_nlink = args->fs_table->entrys[dirs[i].file_id].link_count;
            stbufm->st_mode =  dirs[i].mode; 
            stbufm->st_size = fcfs_get_file_size(args, dirs[i].file_id);

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
    fcfs_dir_entry_t *dirs = fcfs_read_directory(args, fcfs_getattr_cache.fid, &dirs_len);

    fcfs_dir_entry_t *tmp = calloc(1, sizeof(fcfs_dir_entry_t) * (dirs_len + 1));
    memcpy(tmp, dirs, sizeof(fcfs_dir_entry_t) * dirs_len);
    free(dirs);

    int new_fid = fcfs_get_free_fid(args);
    if(new_fid <= 0) {
        return -ENOENT;
    }

    fcfs_alloc(args, new_fid);
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

    fcfs_write_directory(args, fcfs_getattr_cache.fid, tmp, dirs_len + 1);

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
