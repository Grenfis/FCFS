#ifndef FCFS_OPTS_H
#define FCFS_OPTS_H

#include <fuse3/fuse.h>
#include <fcfs_structs.h>

void *
fcfs_init(  struct fuse_conn_info *conn,
            struct fuse_config *cfg);

void
fcfs_destroy(void *args);

int
fcfs_getattr(   const char *path,
                struct stat *stbuf,
                struct fuse_file_info *fi);

int
fcfs_readdir(   const char *path,
                void *buf,
                fuse_fill_dir_t filler,
                off_t offset,
                struct fuse_file_info *fi,
                enum fuse_readdir_flags flags);

int
fcfs_mkdir( const char *path,
            mode_t mode);

int
fcfs_rmdir(const char *path);

int
fcfs_open(  const char *path,
            struct fuse_file_info *fi);

int
fcfs_read(  const char *path,
            char *buf,
            size_t size,
            off_t offset,
            struct fuse_file_info *fi);


typedef struct fcfs_getattr_bentry {
    char *path;
    int fid;
} fcfs_getattr_bentry_t;

#endif
