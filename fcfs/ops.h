#ifndef OPS_H
#define OPS_H

#include <fuse3/fuse.h>
#include <fcfs_structs.h>

void *
ops_init(struct fuse_conn_info *conn, struct fuse_config *cfg);

void
ops_destroy(void *args);

int
ops_getattr(const char *path, struct stat *stbuf, struct fuse_file_info *fi);

int
ops_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi, enum fuse_readdir_flags flags);

int
ops_mkdir(const char *path, mode_t mode);

int
ops_rmdir(const char *path);

int
ops_open(const char *path, struct fuse_file_info *fi);

int
ops_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi);

int
ops_create(const char *path, mode_t mode, struct fuse_file_info *fi);

int
ops_unlink(const char *path);

int
ops_truncate(const char *path, off_t offset, struct fuse_file_info *fi);

int
ops_chown(const char *path, uid_t u, gid_t g, struct fuse_file_info *fi);

int
ops_utimens(const char *path, const struct timespec tv[2], struct fuse_file_info *fi);

#endif
