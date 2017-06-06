#ifndef COMMON_H
#define COMMON_H

#include "../ops.h"
#include "../fcfs.h"
#include "../dev.h"
#include "../utils.h"

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <fuse3/fuse.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#include <debug.h>

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
