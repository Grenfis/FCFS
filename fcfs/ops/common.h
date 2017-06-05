#ifndef COMMON_H
#define COMMON_H

typedef struct fcfs_args fcfs_args_t;

//get main fs data
fcfs_args_t *
fcfs_get_args(void);

//get file if byt path
int
fcfs_get_fid(const char *path);

//get parent file id by path
int
fcfs_get_pfid(const char *path);

#endif
